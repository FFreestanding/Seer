#include <paging/page.h>
#include <paging/pde.h>
#include <paging/pte.h>
#include <kernel_io/vga.h>
#include <kernel_io/memory.h>

/*
    Lunaixsky: 必须为uint8_t，你之前的 uint32_t 取地址计算 
                    KERNEL_MAIN_PAGE_COUNTS = ((uint32_t)((&__kernel_end - &__kernel_start) + 0xfff) >> 12)
                就相当于是两个 uint32_t* 的减法，这个意思就相当于两个 int array 的 index 相减，所以：
                    &__kernel_end - &__kernel_start <==> __kernel_end >> 2 - __kernel_start >> 2
                这就导致你少映射了很多页表
*/


extern uint8_t __kernel_start;
extern uint8_t __kernel_end;

/*
    Lunaixsky: 你的libs产出的 .o 文件没有在linker.ld里面定义对应的 rules，
                所以gcc会使用默认的行为，自动将各个段放到相应的位置，如.text就放到linker.ld里面定义的.text，
                这种行为就恰好导致你的代码可以“正常运行”，但随着日后的开发，这可能是一个隐患。
                使用 boot_text 这个宏变量（定义见同目录下的 bl.h），
                可以让你摆脱这些rules的编写，更加的灵活，linker.ld也更加的清爽
*/

void 
memory_set_tmp(uint8_t* source, uint8_t value, uint32_t length)
{
    while (length--)
    {
        (*source) = value;
        source++;
    }
}

void 
_page_init(uint32_t* ptd_address, uint32_t page_table_size)
{
    // ptd_address = 0x10d000
    // initialize to 0 
    memory_set_tmp((uint8_t*)ptd_address, 0, page_table_size);
    // directory index 0 map to the end+1(uint32) of directory
    PAGE_DIRECTORY_SET(ptd_address, PAGE_DIRECTORY_ADDRESS_32_12(ptd_address + MAX_PAGE_ENTRIES), PAGE_DIRECTORY_PRESENT(1));
    // identity mapping 0-1MB(include VGA)
    for (uint32_t i = 0; i < (0x100000 / ENTRY_MAP_RANGE_NUMBER); i++)//填充了第一个页表的256个元素 共占1MB
    {
        // PAGE_TABLE_SET(ptd_address + MAX_PAGE_ENTRIES + i, PAGE_TABLE_ADDRESS_32_12(i << 12), DEFAULT_PAGE_FLAGS);
        *((uint32_t*)ptd_address + 1024 + i) = (((uint32_t)(i << 12) & 0xfffff000)) | (((1 & 0b1) | ((1 & 0b1) << 1)));
    }
    // identity mapping kernel page init code
    // uint32_t a = (0x100000 / ENTRY_MAP_RANGE_NUMBER) + KERNEL_PAGE_INIT_PAGE_COUNTS;//1048077
    // uint32_t b = KERNEL_PAGE_INIT_PAGE_COUNTS;//1047821
    // uint32_t c = (0x100000 / ENTRY_MAP_RANGE_NUMBER);//256((uint32_t)((&__kernel_end - &__kernel_start) + 0xfff) >> 12)
    for (uint32_t i = 0; i < KERNEL_PAGE_INIT_PAGE_COUNTS; i++)
    {
        PAGE_TABLE_SET(ptd_address + MAX_PAGE_ENTRIES + (0x100000 / ENTRY_MAP_RANGE_NUMBER) + i, PAGE_TABLE_ADDRESS_32_12(0x100000 + (i << 12)), DEFAULT_PAGE_FLAGS);
    }
    
    /* 
        Lunaixsky: 是不是忘记添加 PAGE_TABLE_KERNEL_MAIN_INDEX 的偏移了？
    */
    // mapping kernel main code (set directory)
    // __kernel_start = 0xc010a000
    for (uint32_t i = 0; i < KERNEL_MAIN_PAGE_TABLE_COUNTS; i++)
    {//0x10dc00:	0x00110003 0010f003	0x00000000 00111003
        PAGE_DIRECTORY_SET(
            ptd_address + PDE_INDEX((uint32_t)&__kernel_start) + i, 
            PAGE_TABLE_ADDRESS_32_12(ptd_address + MAX_PAGE_ENTRIES * (i + PAGE_TABLE_KERNEL_MAIN_INDEX)),
            DEFAULT_PAGE_FLAGS
        );
    }
    /* 
        Lunaixsky: 映射的物理地址没有递增【已修复】
     */
    // mapping kernel main code (set second table)
    uintptr_t vaddr = (uintptr_t)&__kernel_start;
    for (uint32_t i = 0; i < KERNEL_MAIN_PAGE_COUNTS; i++)
    {
        PAGE_TABLE_SET(
            ptd_address + PAGE_TABLE_KERNEL_MAIN_INDEX * MAX_PAGE_ENTRIES 
            + PTE_INDEX(vaddr), 
            PAGE_TABLE_ADDRESS_32_12(KERNEL_MAIN_ADDRESS_VIRTUAL_TO_PHYSICAL(vaddr)),
            DEFAULT_PAGE_FLAGS
        );

        // PAGE_TABLE_SET(
        //     ptd_address + PAGE_TABLE_KERNEL_MAIN_INDEX * MAX_PAGE_ENTRIES 
        //     + PTE_INDEX(KERNEL_MAIN_ADDRESS_PHYSICAL_TO_VIRTUAL(vaddr)), 
        //     PAGE_TABLE_ADDRESS_32_12(vaddr),
        //     DEFAULT_PAGE_FLAGS
        // );
        vaddr += 0x1000;
    }
    // self reference
    PAGE_DIRECTORY_SET(
        ptd_address + 1023, 
        PAGE_DIRECTORY_ADDRESS_32_12(ptd_address),
        SELF_REFERENCE_FLAGS
    );
    // vga_putstr(get_vga_manager_instance(),"_page_init ok");
}