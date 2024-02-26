#ifndef __KERNEL_BITSET_H
#define __KERNEL_BITSET_H 1

#include <stdint.h>

typedef struct kernel_bitset
{
    uint8_t* bitset_ptr;
    uint32_t bitset_length;
} kbitset;

kbitset 
kernel_bitset_create(uint8_t* ptr, uint32_t length);

void 
kernel_bitset_set_one_bit(kbitset* k, uint32_t index);

void 
kernel_bitset_unset_one_bit(kbitset* k, uint32_t index);

void 
kernel_bitset_set_chunk(kbitset* k, uint32_t start_index, uint32_t length);

void 
kernel_bitset_unset_chunk(kbitset* k, uint32_t start_index, uint32_t length);

#endif