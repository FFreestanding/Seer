#include <kernel_io/cake_pile.h>
#include "utils/kernel_mathlib.h"
#include "kernel_io/memory.h"
#include <kernel_io/valloc.h>

static char piles_names[][PILE_NAME_MAXLEN] = {"pile_8B",
                                               "pile_16B",
                                               "pile_32B",
                                               "pile_64B",
                                               "pile_128B",
                                               "pile_256B",
                                               "pile_512B",
                                               "pile_1KB",
                                               "pile_2KB",
                                               "pile_4KB",
                                               "pile_8KB"};

static char piles_dma_names[][PILE_NAME_MAXLEN] = {"pile_dma_128B",
                                                   "pile_dma_256B",
                                                   "pile_dma_512B",
                                                   "pile_dma_1KB",
                                                   "pile_dma_2KB",
                                                   "pile_dma_4KB"};

static struct cake_pile* valloc_piles[sizeof(piles_names)/PILE_NAME_MAXLEN];
static struct cake_pile* valloc_piles_dma[sizeof(piles_names)/PILE_NAME_MAXLEN];

void valloc_init()
{
    for (int i = 0; i < sizeof(piles_names)/PILE_NAME_MAXLEN; ++i) {
        int size = 1 << (i + 3);
        valloc_piles[i] = new_cake_pile(piles_names[i],size,
                                        size > 1024 ? 8 : 1, 0);
    }

    // DMA 128B alignment
    for (int i = 0; i < sizeof(piles_dma_names)/PILE_NAME_MAXLEN; ++i) {
        int size = 1 << (i + 7);
        valloc_piles_dma[i] = new_cake_pile(piles_dma_names[i], size,
                                            size > 1024 ? 4 : 1, 1);
    }
}

void*
__valloc(uint32_t size, struct cake_pile** pile_list, uint32_t len, uint32_t boffset)
{
    uint32_t i = ILOG2(size);
    if ((size-(1<<i))!=0)
    {
        i += 1;
    }
    i -= boffset;

    if (i >= len)
        i = 0;

    return piece_alloc(pile_list[i]);
}


void*
valloc(uint32_t size)
{
    if (size > (8*1024)) {
        return NULL;
    }
    return __valloc(size, valloc_piles, sizeof(piles_names)/PILE_NAME_MAXLEN, 3);
}

void*
valloc_dma(uint32_t size)
{
    return __valloc(size, valloc_piles_dma, sizeof(piles_dma_names)/PILE_NAME_MAXLEN, 7);
}

void
__vfree(void* ptr, struct cake_pile** pile_list, uint32_t len)
{
    uint32_t i = 0;
    for (; i < len; i++) {
        if (piece_release(pile_list[i], ptr)) {
            return;
        }
    }
}

void
vfree(void* ptr)
{
    __vfree(ptr, valloc_piles, sizeof(piles_names)/PILE_NAME_MAXLEN);
}

void
vfree_dma(void* ptr)
{
    __vfree(ptr, valloc_piles_dma, sizeof(piles_dma_names)/PILE_NAME_MAXLEN);
}