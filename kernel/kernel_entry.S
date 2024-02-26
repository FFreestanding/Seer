/* 高半核入口点 - 0xC0000000 */

.section .text
    .global kernel_entry
    kernel_entry:
        subl $16, %esp

        call _kernel_init_table

        subl $0x6, %esp
        //load GDT
        movl $_gdt, 2(%esp)
        movw _gdt_limit, %ax
        movw %ax, (%esp)
        lgdt (%esp)
        //load IDT
        movl $_idt, 2(%esp)
        movw _idt_limit, %ax
        movw %ax, (%esp)
        lidt (%esp)

        addl $0x6, %esp

        movw $0x10, %cx
        movw %cx, %es
        movw %cx, %ds
        movw %cx, %fs
        movw %cx, %gs
        movw %cx, %ss

        pushw $0x08
        pushl $_after_load
        retf

    _after_load:

        movl $multiboot_info, (%esp)
        call _bitmap_init

        /* 
            加载新的栈指针，位于0xffbfffff，但因为16字节对齐的需求，低四位清零。
            为什么不是0xffffffff？ 因为0xffc00000 - 0xffffffff 这4MiB的空间用于
            对页表与页目录的循环映射。
        */
        mov $0xffbffff0, %esp

        /* 进入内核 */  
        call _kernel_main

        cli
    j_:
        hlt
        jmp j_