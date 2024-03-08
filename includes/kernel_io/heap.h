#ifndef __HEAP_H
#define __HEAP_H 1
#include <stdint.h>

#define PROLOGUE_TAG 0xffffff00
#define EPILOGUE_TAG 0xfffff000

#define SELF_ALLOCATED 0b1
#define SELF_NOT_ALLOCATED 0b0
#define PREVIOUS_FREE (0b1<<1)
#define PREVIOUS_NOT_FREE (0b0<<1)

#define CHUNK_SELF_ALLOCATED(u32_data) ((u32_data) & 0b1 == 0b1)
#define CHUNK_PREVIOUS_FREE(u32_data) ((u32_data) & 0b10 == 0b10)

#define CHUNK_HEAD(size, flags) ((size)|(flags))
#define CHUNK_TAIL(size, flags) ((size)|(flags))
#define CHUNK_SIZE(u32_data) ((u32_data)&0xfffffff8)

#define ALIGN_8B_UP(x) (((x)+0b111)&0xfffffff8)

typedef struct heap_manager
{
    uint32_t* upper_limit;
    uint32_t* lower_limit;
    uint32_t* chunk_ptr;
} heap_manager;


heap_manager*
get_heap_manager_instance();

void
heap_manager_init(heap_manager*);

uint8_t
heap_grow(heap_manager*);

void *
kmalloc(uint32_t size);

uint8_t
kfree(void* body);

#endif