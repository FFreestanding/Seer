#define __ASM__ 1
#include <boot/multiboot.h>

#define MULTIBOOT_FLAGS MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO
//6=1+5, 1 PageDirectory, 5 PageTables, 4*1024=4K
#define PAGE_TABLE_SIZE    6*(4*1024)

.section .multiboot
    .long MULTIBOOT_MAGIC
    .long MULTIBOOT_FLAGS
    .long CHECKSUM(MULTIBOOT_FLAGS)

.section .bss
    .global multiboot_info
    .align 16
    multiboot_info:
        .skip 4096
    stack_bottom:
        .skip 32768,0
    stack_top:

//configuration of page directory or table
.section .kernel_page_table
    .global _kernel_ptd
    //initialize to 0
    _kernel_ptd:
        .skip PAGE_TABLE_SIZE, 0


.section .text
    .global _init
    .type _init, @function
    _init:
        cld
        cli
        movl $stack_top, %esp
        subl $16, %esp
        //save multiboot information to stack top
        movl $multiboot_info, 4(%esp)
        movl %ebx, (%esp)
        call _save_multiboot_info


        movl $(PAGE_TABLE_SIZE), 4(%esp)
        movl $(_kernel_ptd - 0xC0000000), (%esp)
        call _page_init


        movl (%esp), %eax
        andl $0xfffff000, %eax
        movl %eax, %cr3

        /* 开启分页与地址转换 (CR0.PG=1) 开启虚拟内存，CR0最高位pg设为1 */
        movl %cr0, %eax
        orl $0x80000000, %eax
        movl %eax, %cr0

        addl $16, %esp

        /* 进入高半核！ */
        pushl $kernel_entry
        ret
