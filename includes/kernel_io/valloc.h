#ifndef SEER_VALLOC_H
#define SEER_VALLOC_H

#include <stdint.h>

void valloc_init();

void*
__valloc(uint32_t size, struct cake_pile** pile_list, uint32_t len, uint32_t boffset);

void
__vfree(void* ptr, struct cake_pile** pile_list, uint32_t len);

void*
valloc(uint32_t size);

void*
valloc_dma(uint32_t size);

void
vfree(void* ptr);

void
vfree_dma(void* ptr);

#endif //SEER_VALLOC_H
