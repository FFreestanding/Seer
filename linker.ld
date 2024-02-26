ENTRY(_init)


SECTIONS {
    . = 0x100000;

    .kernel_page_init_text BLOCK(4K) : {
        build/obj/bootloader/*.o (.multiboot)
        build/obj/bootloader/*.o (.text)
    }

    .kernel_page_init_bss BLOCK(4K) : {
        build/obj/bootloader/*.o (.bss)
    }

    .kernel_page_init_data BLOCK(4K) : {
        build/obj/bootloader/*.o (.data)
    }

    .kernel_page_init_rodata BLOCK(4K) : {
        build/obj/bootloader/*.o (.rodata)
    }
    __kernel_page_init_end = ALIGN(4K);

    . += 0xC0000000;

    .text BLOCK(4K) : AT ( ADDR(.text) - 0xC0000000 ) {
        __kernel_start = .;
        EXCLUDE_FILE (build/obj/bootloader/*.o) *(.text)
    }

    .data BLOCK(4k) : AT ( ADDR(.data) - 0xC0000000 ) {
        EXCLUDE_FILE (build/obj/bootloader/*.o) *(.data)
    }

    .rodata BLOCK(4K) : AT ( ADDR(.rodata) - 0xC0000000 ) {
        EXCLUDE_FILE (build/obj/bootloader/*.o) *(.rodata)
    }

    .kernel_page_table_t BLOCK(4K) : AT ( ADDR(.kernel_page_table_t) - 0xC0000000 ) {
        build/obj/bootloader/*.o (.kernel_page_table)
    }

    .bss BLOCK(4K) : AT ( ADDR(.bss) - 0xC0000000 ) {
        EXCLUDE_FILE (build/obj/bootloader/*.o) *(.bss)
    }

    __kernel_end = ALIGN(4K);
    __heap_start = ALIGN(4K);
}