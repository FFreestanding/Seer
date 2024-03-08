#include <kernel_io/vga.h>
#include <boot/multiboot.h>
#include <paging/pmm.h>
#include <paging/vmm.h>
#include <paging/page.h>
#include <gdt/gdt.h>
#include <idt/idt.h>
#include <kernel_io/memory.h>
#include <kernel_io/heap.h>
#include <apic/apic.h>
#include <apic/acpi.h>
#include <kdebug/kdebug.h>
#include <idt/interrupts.h>
#include <apic/ioapic.h>
#include <common.h>
#include <apic/keyboard.h>
#include <device/pci/pci.h>
#include <apic/timer.h>
#include <kdebug/kernel_test.h>
#include <device/sata/ahci.h>
#include <filesystem/ext2.h>

void
_kernel_init_table()
{
    _init_gdt();
    _init_idt();
}

void logh(char *info, char *buff, uint32_t len)
{
    kernel_log(INFO, "----------[%s]----------", info);
    for (uint32_t i = 0; i < len; i++)
    {
        kprintf("%h ", buff[i]);
    }
    kprintf("\n");
    kernel_log(INFO, "------------------------");
}

void
parse_ext2img()
{
    struct hba_port* port = ahci_get_port(0);

    kernel_log(INFO, "super block size: %h", sizeof(struct ext2_superblock));
    char *buffer = kmalloc(512);
    memory_set(buffer, '3', 512);
    int result;

    result = port->device->ops.read_buffer(port, 2, buffer, port->device->block_size);
    if (!result) {
        kprintf("fail to read: %h\n", port->device->last_error);
    }
    parse_superblock(buffer);

    kfree(buffer);
}

void
parse_ext2img_tmp()
{
    struct hba_port* port = ahci_get_port(0);

    char *buffer = kmalloc(512);
    memory_set(buffer, '3', 512);
    int result;

    // 写入第一扇区 (LBA=0)
    result =
      port->device->ops.write_buffer(port, 0, buffer, port->device->block_size);
    if (!result) {
        kprintf("fail to write: %h\n", port->device->last_error);
    }

    memory_set(buffer, '6', port->device->block_size);

    // 读出我们刚刚写的内容！
    result =
      port->device->ops.read_buffer(port, 0, buffer, port->device->block_size);
    kprintf("%h, %h\n", port->regs[HBA_RPxIS], port->regs[HBA_RPxTFD]);
    if (!result) {
        kprintf("fail to read: %h\n", port->device->last_error);
    } else {
        kernel_log(INFO, "success: %s", buffer);
    }

    kfree(buffer);
}

void
_bitmap_init(multiboot_info_t* mb_info)
{
    _interrupt_routine_init();

    multiboot_memory_map_t* memory_map_ptr = (multiboot_memory_map_t*)mb_info->mmap_addr;
    PM_Manager* physical_memory_manager = physical_memory_manager_instance_get();
    pmm_init(physical_memory_manager);
    physical_memory_manager->max_page_numbers = ((mb_info->mem_upper << 10) + 0x100000) >> 12;
    
    for (uint32_t i = 0; i < (mb_info->mmap_length / sizeof(multiboot_memory_map_t)); i++)
    {
        if (memory_map_ptr[i].type == MULTIBOOT_MEMORY_AVAILABLE) {
            // 整数向上取整除法
            pmm_mark_chunk_available(physical_memory_manager, 
            (memory_map_ptr[i].addr_low + 0x0fffU) >> 12, 
            memory_map_ptr[i].len_low >> 12);
        }
    }

    // pmm_mark_chunk_occupied(physical_memory_manager, NULL_POINTER, 1);
    
    // pmm_mark_chunk_occupied(physical_memory_manager, 
    //     KERNEL_MAIN_ADDRESS_VIRTUAL_TO_PHYSICAL((uint32_t)&__kernel_start) >> 12, 
    //     KERNEL_MAIN_PAGE_COUNTS);

    pmm_mark_chunk_occupied(physical_memory_manager, 0, 
        (KERNEL_MAIN_ADDRESS_VIRTUAL_TO_PHYSICAL(&__kernel_end)+0xfff)>>12);

    /*
        Lunaixsky: 忘记转换成 ppn了？
    */
    // pmm_mark_chunk_occupied(physical_memory_manager, 
    //     VGA_START_POINTER, 
    //     VGA_BUFFER_SIZE);

    pmm_mark_chunk_occupied(physical_memory_manager, 
        VGA_START_POINTER >> 12, 
        VGA_BUFFER_SIZE/(4*1024));

    for (uint32_t i = 0; i < (VGA_BUFFER_SIZE >> 12); i++)
    {
        /*
            Lunaixsky: 你这是在 map 什么？
        */
        // vmm_map_page(
        //     (void*)(VGA_START_POINTER + (i << 12)), 
        //     (void*)(VGA_BUFFER_SIZE + (i << 12)), 
        //     DEFAULT_PAGE_FLAGS,
        //     DEFAULT_PAGE_FLAGS
        // );

        vmm_map_page(
            VGA_VIRTUAL_ADDRESS + (i << 12), 
            VGA_START_POINTER + (i << 12), 
            DEFAULT_PAGE_FLAGS,
            DEFAULT_PAGE_FLAGS
        );
    }
    VGA_Manager* v = get_vga_manager_instance();

    vga_manager_init(v);

    // initialize kernel stack
    for (uint32_t i = 0; i < (0x100000 >> 12); i++)
    {
        vmm_alloc_page_entry((void*)((0xFFBFFFFFU - 0x100000 + 1) + (i << 12)), 
                                DEFAULT_PAGE_FLAGS, DEFAULT_PAGE_FLAGS);
    }

    // initialize kernel heap
    heap_manager_init(get_heap_manager_instance());
//    save_debug_info(mb_info->mods_addr);
    for (uint32_t i = 0; i < 3; i++)
    {
        vmm_unmap_page((void*)(i << 12));
    }

    // 锁定所有系统预留页（内存映射IO，ACPI之类的），并且进行1:1映射
    multiboot_memory_map_t* mmaps = (multiboot_memory_map_t *) mb_info->mmap_addr;
    uint32_t map_size = mb_info->mmap_length / sizeof(multiboot_memory_map_t);
    for (unsigned int i = 0; i < map_size; i++) {
        multiboot_memory_map_t mmap = mmaps[i];
        if (mmap.type == MULTIBOOT_MEMORY_AVAILABLE) {
            continue;
        }
        uint8_t* pa = mmap.addr_low & 0xFFFFF000UL;
        uint32_t pg_num = CEIL(mmap.len_low, 12);
        for (uint32_t j = 0; j < pg_num; j++)
        {
            vmm_map_page((uint32_t) (pa + (j << 12)), (uint32_t) (pa + (j << 12)),
                         DEFAULT_PAGE_FLAGS, DEFAULT_PAGE_FLAGS);
        }
    }

    _acpi_init(mb_info);
    kernel_log(INFO, "acpi_init OVER");

    uint32_t ioapic_addr = get_acpi_context_instance()->ioapic->ioapic_address;
    pmm_mark_chunk_occupied(physical_memory_manager, APIC_BASE_PHYSICAL_ADDRESS>>12, 1);
    pmm_mark_chunk_occupied(physical_memory_manager, ioapic_addr>>12, 1);

    _assert(
        APIC_BASE_VIRTUAL_ADDRESS == vmm_map_page(APIC_BASE_VIRTUAL_ADDRESS, 
                                                APIC_BASE_PHYSICAL_ADDRESS, 
                                                DEFAULT_PAGE_FLAGS, 
                                                DEFAULT_PAGE_FLAGS),
            "APIC_BASE_VIRTUAL_ADDRESS MAP ERROR"
    );

    kernel_log(INFO, "ioapic address %h", ioapic_addr);
    _assert(
        IOAPIC_BASE_VIRTUAL_ADDRESS == vmm_map_page(IOAPIC_BASE_VIRTUAL_ADDRESS, 
                                                    ioapic_addr, DEFAULT_PAGE_FLAGS, 
                                                    DEFAULT_PAGE_FLAGS), 
        "IOAPIC_BASE_VIRTUAL_ADDRESS MAP ERROR"
    );

    _apic_init();
    kernel_log(INFO, "apic init OVER");
    
    _ioapic_init();
    kernel_log(INFO, "ioapic init OVER");

    _rtc_init();
    kernel_log(INFO, "rtc init OVER");
    
    _timer_init();
    kernel_log(INFO, "timer init OVER");    

    _ps2_controller_init();
    kernel_log(INFO, "ps2 controller init OVER");

    pci_init();
    kernel_log(INFO, "prob pci device OVER");

    ahci_init();
    kernel_log(INFO, "ahci device init OVER");

    // ahci_list_device();

    // pci_print_device();

    parse_ext2img();

    while (1);
}

void
_kernel_main()
{
    // 清除 __kernel_page_init 与前1MiB的映射
    // uint32_t __kernel_page_init_pg_count = ((uint32_t)(&__kernel_page_init_end)) >> 12;

    // for (uint32_t i = 1; i < __kernel_page_init_pg_count; i++)
    // {
    // kernel_log(WARN, "%h", i);
    // vmm_unmap_page((void*)(i << 12));
    // }

    vga_clear(get_vga_manager_instance());

    // test_heap_management();
    // test_interrupt();
    test_clocks();

    char* logo =
            " .oooooo..o oooooooooooo oooooooooooo ooooooooo.   \n"
            "d8P'    `Y8 `888'     `8 `888'     `8 `888   `Y88. \n"
            "Y88bo.       888          888          888   .d88' \n"
            " `\"Y8888o.   888oooo8     888oooo8     888ooo88P'  \n"
            "     `\"Y88b  888    \"     888    \"     888`88b.    \n"
            "oo     .d8P  888       o  888       o  888  `88b.  \n"
            "8\"\"88888P'  o888ooooood8 o888ooooood8 o888o  o888o \n"
            "                                                   \n";
    kernel_log(INFO, "%s", logo);

    while (1);
}
