#include <kernel_io/heap.h>
#include <paging/vmm.h>
#include <paging/page.h>
#include <kernel_io/memory.h>

heap_manager*
get_heap_manager_instance()
{
    static heap_manager hm;
    return &hm;
}

extern uint8_t __heap_start;

void
heap_manager_init(heap_manager* hm)
{
    hm->lower_limit = &__heap_start;
    hm->upper_limit = &__heap_start + 0x1000 - 4;
    vmm_alloc_page_entry(hm->lower_limit, DEFAULT_PAGE_FLAGS, DEFAULT_PAGE_FLAGS);
    hm->chunk_ptr = hm->lower_limit;
    *(hm->lower_limit) = CHUNK_HEAD(0x1000, PREVIOUS_NOT_FREE | SELF_NOT_ALLOCATED);
    *(hm->upper_limit) = CHUNK_TAIL(0x1000, PREVIOUS_NOT_FREE | SELF_NOT_ALLOCATED);
}

uint8_t
heap_grow(heap_manager* hm)
{
    uint32_t tail = *(hm->upper_limit);
    *(hm->upper_limit - CHUNK_SIZE(tail)/4 + 1) = CHUNK_HEAD(CHUNK_SIZE(tail)+0x1000, PREVIOUS_NOT_FREE | SELF_NOT_ALLOCATED);
    if (!vmm_alloc_page_entry(hm->upper_limit + 1, DEFAULT_PAGE_FLAGS, DEFAULT_PAGE_FLAGS))
    {
        return NULL;
    }
    hm->upper_limit += 0x1000/4;
    *(hm->upper_limit) = CHUNK_TAIL(CHUNK_SIZE(tail)+0x1000, PREVIOUS_NOT_FREE | SELF_NOT_ALLOCATED);
    return 1;
}

uint32_t*
kmalloc(uint32_t size)
{
    uint32_t actual_size = ALIGN_8B_UP(size);
    if (!(size % 8))
    {
        actual_size += 8;
    }
    
    heap_manager* hm = get_heap_manager_instance();
    while (CHUNK_SELF_ALLOCATED(*(hm->chunk_ptr)) && hm->chunk_ptr < hm->upper_limit)
    {
        hm->chunk_ptr += CHUNK_SIZE(*hm->chunk_ptr)/4;
    }

    if (hm->chunk_ptr >= hm->upper_limit)
    {
        if (heap_grow(hm))
        {
            return kmalloc(size);
        }
        else
        {
            return NULL_POINTER;
        } 
    }
    else if (CHUNK_SIZE(*(hm->chunk_ptr)) >= actual_size)
    {

        uint32_t t = CHUNK_SIZE(*(hm->chunk_ptr))-actual_size;
        //判断是否切割
        if (t >= 8)
        {
            *(hm->chunk_ptr) = CHUNK_HEAD(actual_size, SELF_ALLOCATED | PREVIOUS_NOT_FREE);
            // hm->chunk_ptr += actual_size/4;
            *(hm->chunk_ptr + actual_size/4) = CHUNK_HEAD(t, SELF_NOT_ALLOCATED | PREVIOUS_NOT_FREE);
            *(hm->chunk_ptr + actual_size/4 +t/4-1) = CHUNK_TAIL(t, SELF_NOT_ALLOCATED | PREVIOUS_NOT_FREE);
        }
        else
        {
            *(hm->chunk_ptr) = CHUNK_HEAD(actual_size, SELF_ALLOCATED | PREVIOUS_NOT_FREE);
        }
        return hm->chunk_ptr + 1;
    }
    else if (CHUNK_SIZE(*(hm->chunk_ptr)) < actual_size)
    {
        if (heap_grow(hm))
        {
            return kmalloc(size);
        }
        else
        {
            return NULL_POINTER;
        } 
    }
    else
    {
        return NULL_POINTER;
    }
    
}

uint8_t 
kfree(void* body)
{
    uint32_t* chunk_p = body - 1;
    //get self size
    uint32_t s = CHUNK_SIZE(*chunk_p);
    heap_manager* hm = get_heap_manager_instance();
    if (s<8 || chunk_p>=(-s) || (uint32_t)chunk_p&(0x7) || (*chunk_p)&0b100)
    {
        return 0;
    }
    // previous and next is free
    if (CHUNK_PREVIOUS_FREE(*chunk_p) && !CHUNK_SELF_ALLOCATED(*(chunk_p + s/4)))
    {
        // get previous size
        uint32_t p = CHUNK_SIZE(*(chunk_p-1));
        // get next size
        uint32_t n = CHUNK_SIZE(*(chunk_p+s/4));
        // set previous head
        *(chunk_p - p/4) = CHUNK_HEAD(p+s+n, PREVIOUS_NOT_FREE | SELF_NOT_ALLOCATED);
        // set next tail
        *(chunk_p + (s + n)/4 - 1) = CHUNK_TAIL(p+s+n, PREVIOUS_NOT_FREE | SELF_NOT_ALLOCATED);

        hm->chunk_ptr = chunk_p - p/4;
    }
    // only previous is free
    else if (CHUNK_PREVIOUS_FREE(*chunk_p) && CHUNK_SELF_ALLOCATED(*(chunk_p + s/4)))
    {
        // get previous size
        uint32_t p = CHUNK_SIZE(*(chunk_p-1));
        // set previous head
        *(chunk_p - p/4) = CHUNK_HEAD(p+s, PREVIOUS_NOT_FREE | SELF_NOT_ALLOCATED);
        // set self tail
        *(chunk_p + s/4 - 1) = CHUNK_TAIL(p+s, PREVIOUS_NOT_FREE | SELF_NOT_ALLOCATED);

        hm->chunk_ptr = chunk_p - p/4;
    }
    // only next is free
    else if (!CHUNK_PREVIOUS_FREE(*chunk_p) && !CHUNK_SELF_ALLOCATED(*(chunk_p + s/4)))
    {
        // get next size
        uint32_t n = CHUNK_SIZE(*(chunk_p+s/4));
        // set self head
        *(chunk_p) = CHUNK_HEAD(s+n, PREVIOUS_NOT_FREE | SELF_NOT_ALLOCATED);
        // set tail tail
        *(chunk_p + (s+n)/4 - 1) = CHUNK_TAIL(s, PREVIOUS_NOT_FREE | SELF_NOT_ALLOCATED);

        hm->chunk_ptr = chunk_p;
    }
    // previous and next is allocated
    else if (!CHUNK_PREVIOUS_FREE(*chunk_p) && CHUNK_SELF_ALLOCATED(*(chunk_p + s/4)))
    {
        // set self head
        *(chunk_p) = CHUNK_HEAD(s, PREVIOUS_NOT_FREE | SELF_NOT_ALLOCATED);
        // set self tail
        *(chunk_p + s/4 - 1) = CHUNK_TAIL(s, PREVIOUS_NOT_FREE | SELF_NOT_ALLOCATED);
        hm->chunk_ptr = chunk_p;
    }

    return 1;
}