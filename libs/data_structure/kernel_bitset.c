#include <data_structure/kernel_bitset.h>
#include <utils/kernel_mathlib.h>
#include "kernel_io/memory.h"
#include "paging/pmm.h"

kbitset 
kernel_bitset_create(uint8_t* ptr, uint32_t length)
{
    kbitset k;
    k.bitset_ptr = ptr;
    k.bitset_length = length;
    return k;
}


void 
kernel_bitset_set_one_bit(kbitset* k, uint32_t index)
{
    k->bitset_ptr[index/8] |= (1 << (index%8));
}

void 
kernel_bitset_unset_one_bit(kbitset* k, uint32_t index)
{
    // k->bitset_ptr[index/8] &= (0 << (index%8));
    k->bitset_ptr[index/8] &= ~(1 << (index%8));
}

void 
kernel_bitset_set_chunk(kbitset* k, uint32_t start_index, uint32_t length)
{
    // if (length >= 16)
    // {
    //     uint8_t head, tail;
    //     uint32_t body_count;
    //     head = 0xff << (start_index % 8);
    //     k->bitset_ptr[start_index] |= head;

    //     kbitset head_bit_set = kernel_bitset_create((uint8_t)head, 8);
    //     uint8_t head_set_count = kernel_bitset_count(&head_bit_set, start_index, 8);

    //     body_count = (length - head_set_count) / 8;
    //     for (uint32_t i = 0; i < body_count; i++)
    //     {
    //         k->bitset_ptr[start_index + 1 + i] = 0xff;
    //     }
        
    //     tail = (length - head_set_count) % 8;
    //     k->bitset_ptr[start_index + 1 + body_count] |= kernel_pow(2, tail) - 1;    
    // }
    // else
    // {
    //     for (uint32_t i = 0; i < length; i++)
    //     {
    //         kernel_bitset_set_one_bit(k, start_index+i);
    //     }
    // }
    for (uint32_t i = 0; i < length; i++)
    {
        kernel_bitset_set_one_bit(k, start_index+i);
    }
}

void 
kernel_bitset_unset_chunk(kbitset* k, uint32_t start_index, uint32_t length)
{
    // if (length >= 16)
    // {
    //     uint8_t head, tail;
    //     uint32_t body_count;
    //     head = 0xff << (start_index % 8);
    //     k->bitset_ptr[start_index] &= (~head);

    //     kbitset head_bit_set = kernel_bitset_create((uint8_t)head, 8);
    //     uint8_t head_set_count = kernel_bitset_count(&head_bit_set, start_index, 8);

    //     body_count = (length - head_set_count) / 8;
    //     for (uint32_t i = 0; i < body_count; i++)
    //     {
    //         k->bitset_ptr[start_index + 1 + i] = 0x00;
    //     }
        
    //     uint32_t tail_count = (length - head_set_count) % 8;
    //     tail = ~(kernel_pow(2, tail_count) - 1);
    //     k->bitset_ptr[start_index + 1 + body_count] &= tail;    
    // }
    // else
    // {
    //     for (uint32_t i = 0; i < length; i++)
    //     {
    //         kernel_bitset_set_one_bit(k, start_index+i);
    //     }
    // }
    for (uint32_t i = 0; i < length; i++)
    {
        kernel_bitset_unset_one_bit(k, start_index+i);
    }
}

uint32_t
kernel_bitset_get_one_bit(kbitset* k, uint32_t index)
{
    uint32_t byte_index = index / 8;
    uint8_t byte = *(k->bitset_ptr + byte_index);
    uint8_t bit_offset = index % 8;
    return (byte>>bit_offset)&0b1;
}

uint32_t find_unset_bit_index(kbitset *k)
{
    uint32_t i = 0;
    while (i < k->bitset_length)
    {
        if(kernel_bitset_get_one_bit(k, i)==0)
        {
            kernel_bitset_set_one_bit(k, i);
            return i;
        }
        ++i;
    }
    return 0;
}