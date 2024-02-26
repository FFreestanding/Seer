#ifndef __VMM_H
#define __VMM_H 1

#include <stdint.h>
/* Virtual Memory Manager */
#define NULL 0
#define NULL_POINTER 0
#define VIRTUAL_ADDRESS(pd_offset, pt_offset, offset) (uint32_t*)((pd_offset) << 22 | (pt_offset) << 12 | (offset))
#define PAGE_TABLE_VIRTUAL_ADDRESS(pd_offset) (uint32_t*)(0xFFC00000U | ((pd_offset & 0xfff) << 12))
#define PAGE_OFFSET(address) ((address)&0xfff)

void*
vmm_map_page(uint32_t virtual_address, uint32_t physical_address, uint32_t directory_flags, uint32_t table_flags);

void*
vmm_alloc_page_entry(void* vpn, uint32_t directory_flags, uint32_t table_flags);

void
vmm_unmap_page(void* vpn);


#endif