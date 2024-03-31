#include <kernel_io/cake_pile.h>
#include "common.h"
#include "kernel_io/memory.h"
#include "paging/pmm.h"
#include "paging/vmm.h"

struct cake_piles_manager main_cake_piles_mgr;

void cake_allocator_init()
{
    // initialize main cake piles
    // its name is seer_cake_piles
    char *main_piles_name = "Seer_main_pile";
    llist_init_head(&main_cake_piles_mgr.piles_list);
    cake_pile_init(&main_cake_piles_mgr.main_piles, main_piles_name,
                   sizeof(struct cake_pile), 1, 0);
}

struct cake_piece *new_cake_pile(char *pile_name, uint32_t piece_size,
                                 uint32_t page_per_cake, uint32_t flags)
{
    struct cake_pile *p = (struct cake_pile *) piece_alloc(&main_cake_piles_mgr.main_piles);
    cake_pile_init(p, pile_name, piece_size, page_per_cake, flags);
    return p;
}

void cake_pile_init(struct cake_pile *pile, char *name,
                    uint32_t piece_size, uint32_t page_per_cake, uint32_t flags)
{
    piece_size = ROUNDUP(piece_size, sizeof(uint32_t));
    *pile = (struct cake_pile) {
        .piece_size = piece_size,
        .offset = sizeof(uint32_t),
        .cakes_count = 0,
        .pieces_per_cake = (page_per_cake*0x1000)/(piece_size + SIZEOF_PIECE_INDEX),
        .page_per_cake = page_per_cake
    };

    uint32_t free_list_size = pile->pieces_per_cake * SIZEOF_PIECE_INDEX;
    pile->offset = ROUNDUP(free_list_size + sizeof(struct cake), sizeof(uint32_t));
    SET_PILE_NAME(pile, name);

    llist_init_head(&pile->free);
    llist_init_head(&pile->full);
    llist_init_head(&pile->partial);
    llist_append(&main_cake_piles_mgr.piles_list, &pile->other_piles);
}

struct cake *new_cake(struct cake_pile* pile)
{
    uint32_t pa = (uint32_t)pmm_alloc_pages(pile->page_per_cake, 0);
    ASSERT(pa, "new_cake");
    struct cake *c = vmm_map_pages(pa, pa,pile->page_per_cake);
    if (c == NULL_POINTER || pile->pieces_per_cake==0)
    {
        return NULL_POINTER;
    }

    c->first_piece = (void *)((uint32_t)c + pile->offset);
    c->next_free = 0;
    ++pile->cakes_count;

    kernel_log(INFO, "Before pile->pieces_per_cake-1: %u", pile->pieces_per_cake-1);
    for (uint32_t i = 0; i < pile->pieces_per_cake-1; ++i) {
        c->free_list[i] = (int32_t) i + 1;
    }
    kernel_log(INFO, "After pile->pieces_per_cake-1: %u", pile->pieces_per_cake-1);
    kernel_log(INFO, "Pass init free_list");

    c->free_list[pile->pieces_per_cake] = FREE_LIST_END;
    llist_append(&pile->free, &c->other_cakes);

    return c;
}


void *piece_alloc(struct cake_pile *pile)
{
    struct cake *pos, *n;
    if (!llist_empty(&pile->partial)) {
        pos = list_entry(pile->partial.next, typeof(*pos), other_cakes);
    } else if (llist_empty(&pile->free)) {
        pos = new_cake(pile);
    } else {
        pos = list_entry(pile->free.next, typeof(*pos), other_cakes);
    }

    if (NULL_POINTER==pos)
    {
        return NULL;
    }

    uint32_t found_index = pos->next_free;
    pos->next_free = pos->free_list[found_index];
    pos->used_pieces++;
    pile->alloced_pieces++;

    llist_delete(&pos->other_cakes);
    if (pos->free_list[pos->next_free] == FREE_LIST_END) {
        llist_append(&pile->full, &pos->other_cakes);
    } else {
        llist_append(&pile->partial, &pos->other_cakes);
    }

    void *ptr = (void *)(pos->first_piece + found_index * pile->piece_size);

    return ptr;
}

uint32_t piece_release(struct cake_pile *pile, void *area)
{
    uint32_t piece_index;
    size_t dsize = 0;
    struct cake *pos, *n;
    struct llist_header* hdrs[2] = { &pile->full, &pile->partial };

    for (size_t i = 0; i < 2; i++) {
        llist_for_each(pos, n, hdrs[i], other_cakes)
        {
            if (pos->first_piece > area) {
                continue;
            }
            dsize = (uint32_t)(area - pos->first_piece);
            piece_index = dsize / pile->piece_size;
            if (piece_index < pile->pieces_per_cake) {
                goto found;
            }
        }
    }

    return 0;

found:
    if ((dsize % pile->piece_size)!=0)
    {
        kernel_log(ERROR, "(dsize % pile->piece_size)!=0");
    }

    pos->free_list[piece_index] = pos->next_free;
    pos->next_free = piece_index;

    if (pos->free_list[piece_index]==pos->next_free)
    {
        kernel_log(ERROR, "Double Free on free_list");
    }

    pos->used_pieces--;
    pile->alloced_pieces--;

    llist_delete(&pos->other_cakes);
    if (!pos->used_pieces) {
        llist_append(&pile->free, &pos->other_cakes);
    } else {
        llist_append(&pile->partial, &pos->other_cakes);
    }

    *((uint32_t *)area) = 0xdeadbeaf;

    return 1;
}