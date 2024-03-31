#include <paging/pmm.h>
#include <data_structure/kernel_bitset.h>
#include <paging/vmm.h>
#include <kernel_io/valloc.h>
#include "kernel_io/memory.h"

static PM_Manager pm_mgr;

void
pmm_init(uint32_t max)
{
    pm_mgr.page_lookup_start = 1;
    pm_mgr.max_page_index = max;
    memory_set(pm_mgr.table, 0, sizeof(pm_mgr.table));
    for (uint32_t i = 0; i < PM_TABLE_MAX_SIZE; ++i) {
        pm_mgr.table[i].reference_counts = 1;
    }
}

void
pmm_mark_chunk_occupied
(uint32_t start_index, uint32_t page_count)
{
    for (uint32_t i = 0; i < page_count; ++i) {
        if (pm_mgr.table[start_index+i].reference_counts == 0xffffffff) { continue; }
        ++pm_mgr.table[start_index+i].reference_counts;
    }
}

void
pmm_mark_chunk_available
(uint32_t start_index, uint32_t page_count)
{
    for (uint32_t i = 0; i < page_count; ++i) {
        if (pm_mgr.table[start_index+i].reference_counts == 0) { continue; }
        else { --pm_mgr.table[start_index+i].reference_counts; }
    }
}

void *
pmm_alloc_page_entry()
{
    /*
        Lunaixsky: ???
    */
    // pmm_init(pm_mgr);
    
    uint32_t start_index = pm_mgr.page_lookup_start;

    void *result = pmm_find_page_entry(pm_mgr.max_page_index);

    if (result!=NULL_POINTER)
    {
        return result;
    }
    else
    {
        pm_mgr.page_lookup_start = 1;
        void *r = pmm_find_page_entry(start_index);
        if (r!=NULL_POINTER)
        {
            return r;
        }
        else
        {
            return NULL_POINTER;
        }
    }
}


void *
pmm_alloc_pages(uint32_t page_num, uint32_t attr)
{
    uint32_t p1 = 1;
    uint32_t p2 = 1;
    while (p2 < pm_mgr.max_page_index && p2 - p1 < page_num) {
        (pm_mgr.table[p2].reference_counts == 0) ? (p2++) : (p1 = ++p2);
    }
    if (p2 == pm_mgr.max_page_index && p2 - p1 < page_num) {
        return NULL_POINTER;
    }
    pmm_mark_chunk_occupied(p1, page_num);
    return (void *) (p1 << 12);
}


// RETURN BITMAP index if found, else return 0
void *
pmm_find_page_entry(uint32_t upper_limit)
{
    for (uint32_t i = pm_mgr.page_lookup_start; i < upper_limit; ++i) {
        if (pm_mgr.table[i].reference_counts==0)
        {
            pm_mgr.page_lookup_start = i;
            return (void *)(i<<12);
        }
    }
    return NULL_POINTER;
}

