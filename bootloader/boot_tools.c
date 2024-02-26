#include <boot/boot_tools.h>
#include <kernel_io/vga.h>

void 
memory_copy_tmp(uint8_t* source, uint8_t* destination, uint32_t length)
{
    while (length--)
    {
        (*destination) = (*source);
        source++;
        destination++;
    }
}

void 
_save_multiboot_info(multiboot_info_t* info, uint8_t* destination) {
    
    /*
    destination -> $mb_info
    info -> ebx
    */
    uint32_t current = 0;
    
    /*
    复制主表
    */
    memory_copy_tmp((uint8_t*)info, destination, sizeof(multiboot_info_t));

    current += sizeof(multiboot_info_t);
    
    ((multiboot_info_t*) destination)->mmap_addr = (uintptr_t)destination + current;
    /*
    复制子表1
    */
    memory_copy_tmp((uint8_t*)(info->mmap_addr), destination + current, info->mmap_length);
    current += info->mmap_length;

    if (present(info->flags, MULTIBOOT_INFO_DRIVE_INFO))
    {
        ((multiboot_info_t*) destination)->drives_addr = (uintptr_t)destination + current;
        /*
        复制子表2
        */
        memory_copy_tmp((uint8_t*)(info->drives_addr), destination + current, info->drives_length);
    }

}