#ifndef __PMM_H
#define __PMM_H 1
#include <stdint.h>
#include <paging/page.h>
/* Physical Memory Manager */

#define PM_BMP_MAX_SIZE (PAGE_COUNTS_SUM / 8)
#define PAGE_OCCUPOED 0
#define PAGE_AVAILABLE 1

typedef struct Physical_Memory_Manager
{
    uint32_t max_page_numbers;
    uint8_t pm_bitmap[PM_BMP_MAX_SIZE];
    uint32_t page_lookup_start;
} PM_Manager;

PM_Manager*
physical_memory_manager_instance_get();

void
pmm_init(PM_Manager* physical_memory_manager);

void
pmm_mark_chunk_occupied
(PM_Manager* p, uint32_t start_index, uint32_t page_count);

void
pmm_mark_chunk_available
(PM_Manager* p, uint32_t start_index, uint32_t page_count);

void*
pmm_alloc_page_entry();

uint32_t
pmm_find_page_entry(PM_Manager* p, uint32_t upper_limit);

void*
pmm_alloc_pages(uint32_t counts);

uint32_t
pmm_find_pages(PM_Manager* p, uint32_t upper_limit, uint32_t counts);

#endif