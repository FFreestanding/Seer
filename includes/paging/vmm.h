#ifndef __VMM_H
#define __VMM_H 1

#include <stdint.h>
#include "pte.h"
#include "pde.h"
/* Virtual Memory Manager */
#define NULL 0
#define NULL_POINTER 0
#define VIRTUAL_ADDRESS(pd_offset, pt_offset, offset) (uint32_t*)((pd_offset) << 22 | (pt_offset) << 12 | (offset))
#define PAGE_TABLE_VIRTUAL_ADDRESS(pd_offset) (uint32_t*)(0xFFC00000U | ((pd_offset & 0xfff) << 12))
#define PAGE_OFFSET(address) ((address)&0xfff)
#define VIRTUAL_ADDR_TO_PHYSICAL_ADDR(vaddr) (vaddr)

static inline
uint32_t vaddr_to_paddr(uint32_t vaddr)
{
    uint32_t page_directory_offset = PDE_INDEX(vaddr);
    uint32_t* page_table_pointer = PAGE_TABLE_VIRTUAL_ADDRESS(page_directory_offset);
    uint32_t page_table_offset = PTE_INDEX(vaddr);
    return (*(page_table_pointer + page_table_offset) & (~0xfff)) + (vaddr&0xfff);
}

void*
vmm_map_page(uint32_t virtual_address, uint32_t physical_address, uint32_t directory_flags, uint32_t table_flags);

void*
vmm_alloc_page_entry(void* vpn, uint32_t directory_flags, uint32_t table_flags);

void
vmm_unmap_page(void* vpn);

void*
vmm_alloc_pages(void* vpn, uint32_t directory_flags, uint32_t table_flags, uint32_t counts);

void*
vmm_map_pages(uint32_t virtual_address, uint32_t physical_address, uint32_t directory_flags, uint32_t table_flags, uint32_t counts);

#endif