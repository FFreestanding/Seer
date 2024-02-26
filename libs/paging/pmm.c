#include <paging/pmm.h>
#include <data_structure/kernel_bitset.h>
#include <paging/vmm.h>



PM_Manager*
physical_memory_manager_instance_get()
{
    // static p->pm_bitmap[] initialize to 0(PAGE_OCCUPOED)
    static PM_Manager p;
    return &p;
}

void
pmm_init(PM_Manager* physical_memory_manager)
{
    physical_memory_manager->page_lookup_start = 0;
    physical_memory_manager->max_page_numbers = 0;
}

void
pmm_mark_chunk_occupied
(PM_Manager* p, uint32_t start_index, uint32_t page_count)
{
    kbitset k = kernel_bitset_create(p->pm_bitmap, page_count);
    kernel_bitset_unset_chunk(&k, start_index, page_count);
}

void
pmm_mark_chunk_available
(PM_Manager* p, uint32_t start_index, uint32_t page_count)
{
    kbitset k = kernel_bitset_create(p->pm_bitmap, page_count);
    kernel_bitset_set_chunk(&k, start_index, page_count);
}

void*
pmm_alloc_page_entry()
{
    PM_Manager* p = physical_memory_manager_instance_get();
    /*
        Lunaixsky: ???
    */
    // pmm_init(p);
    
    uint32_t start_index = p->page_lookup_start;

    uint32_t result = pmm_find_page_entry(p, p->max_page_numbers);

    if (result!=NULL)
    {
        return (void*)(result<<12);
    }
    else
    {
        uint32_t r = pmm_find_page_entry(p, start_index);
        if (r!=NULL)
        {
            return (void*)(r<<12);
        }
        else
        {
            return NULL_POINTER;
        }
    }
}


// RETURN BITMAP index if found, else return 0
uint32_t
pmm_find_page_entry(PM_Manager* p, uint32_t upper_limit)
{
    while (p->pm_bitmap[p->page_lookup_start >> 3] == 0x00
        && p->page_lookup_start <= upper_limit)
    {
        p->page_lookup_start += 8-(p->page_lookup_start % 8);
    }

    if (p->page_lookup_start > upper_limit)
    {
        p->page_lookup_start = 0;
        return NULL;
    }
    else
    {
        uint8_t b = p->pm_bitmap[p->page_lookup_start / 8];
        uint8_t count = 0;
        for (uint32_t i = 0; i < 8 && b != (b | 0x1) && b > 0; i++)
        {
            ++count;
            b >>= 1;
        }

        return p->page_lookup_start + count;
    }
    
}