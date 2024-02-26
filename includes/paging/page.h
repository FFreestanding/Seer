#ifndef __PAGE_H
#define __PAGE_H 1

#include <stdint.h>
#include <paging/pte.h>
#include <paging/pde.h>

extern uint8_t __kernel_page_init_end;
extern uint8_t __kernel_start;
extern uint8_t __kernel_end;
//4G needs 0x100000 pages
#define PAGE_COUNTS_SUM 0x100000
#define MAX_PAGE_ENTRIES 1024
// one entry's size is 4 Byte
#define PAGE_ENTRY_SIZE 4
#define ENTRY_MAP_RANGE_NUMBER 0x1000
//use table index 1(1-5) for initialization
#define PAGE_TABLE_INIT_INDEX 1
//use table index 2-4(1-5) for kernel main
#define PAGE_TABLE_KERNEL_MAIN_INDEX 2
//use table index 5(1-5) for stack
#define PAGE_TABLE_STACK_INDEX 5

#define PAGE_COUNTS(size) ((uint32_t)(size + 0xfff) >> 12)

#define KERNEL_PAGE_INIT_SIZE ((uint32_t)&__kernel_page_init_end - 0x100000)
#define KERNEL_PAGE_INIT_PAGE_COUNTS PAGE_COUNTS(KERNEL_PAGE_INIT_SIZE)

#define KERNEL_MAIN_ADDRESS_VIRTUAL_TO_PHYSICAL(address) ((uint32_t)(address) - 0xc0000000)
#define KERNEL_MAIN_PAGE_TABLE_COUNTS 3

#define KERNEL_MAIN_SIZE (&__kernel_end - &__kernel_start)
#define KERNEL_MAIN_PAGE_COUNTS PAGE_COUNTS(KERNEL_MAIN_SIZE)

#define DEFAULT_PAGE_FLAGS (PAGE_TABLE_PRESENT(1) | PAGE_TABLE_READ_WRITE(1))
#define SELF_REFERENCE_FLAGS (DEFAULT_PAGE_FLAGS | PAGE_DIRECTORY_CACHE_DISABLE(1))

void 
_page_init(uint32_t* ptd_address, uint32_t page_table_size);

#endif