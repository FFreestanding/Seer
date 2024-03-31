#ifndef __PMM_H
#define __PMM_H 1
#include <stdint.h>
#include <paging/page.h>
/* Physical Memory Manager */

#define PM_TABLE_MAX_SIZE (1024 * 1024)

typedef struct pm_table {
    uint32_t reference_counts;
    uint32_t attribute;
} pm_table_t;

typedef struct Physical_Memory_Manager
{
    uint32_t max_page_index;
    pm_table_t table[PM_TABLE_MAX_SIZE];
    uint32_t page_lookup_start;
} PM_Manager;

/*PM_Manager*
physical_memory_manager_instance_get();*/

void
pmm_init(uint32_t max);

void
pmm_mark_chunk_occupied
(uint32_t start_index, uint32_t page_count);

void
pmm_mark_chunk_available
(uint32_t start_index, uint32_t page_count);

void *
pmm_alloc_page_entry();

void *
pmm_find_page_entry(uint32_t upper_limit);

void *
pmm_alloc_pages(uint32_t counts, uint32_t attr);

#endif