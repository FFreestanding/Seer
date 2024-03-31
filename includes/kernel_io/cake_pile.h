#ifndef SEER_CAKE_PILE_H
#define SEER_CAKE_PILE_H

#include <data_structure/llist.h>
#include <stdint.h>

#define SIZEOF_PIECE_INDEX 4
#define PILE_NAME_MAXLEN 16
#define FREE_LIST_END -1

#define SET_PILE_NAME(pile, name)     memory_copy(name, pile->pile_name, PILE_NAME_MAXLEN); \
                                      pile->pile_name[PILE_NAME_MAXLEN] = '\0';

struct cake {
    struct llist_header other_cakes;
    void *first_piece;
    unsigned int used_pieces;
    unsigned int next_free;
    int32_t free_list[0];
};

struct cake_pile {
    struct llist_header other_piles;
    struct llist_header full;
    struct llist_header partial;
    struct llist_header free;
    uint32_t offset;
    uint32_t piece_size;
    uint32_t cakes_count;
    uint32_t alloced_pieces;
    uint32_t pieces_per_cake;
    uint32_t page_per_cake;
    char pile_name[PILE_NAME_MAXLEN+1];
};

struct cake_piles_manager {
    struct cake_pile main_piles;
    struct llist_header piles_list;
};

void cake_pile_init(struct cake_pile *pile, char *name,
        uint32_t piece_size, uint32_t page_per_cake, uint32_t flags);

struct cake *new_cake(struct cake_pile* pile);

struct cake_piece *new_cake_pile(char *pile_name, uint32_t piece_size,
                   uint32_t page_per_cake, uint32_t flags);

void *piece_alloc(struct cake_pile *pile);

uint32_t piece_release(struct cake_pile *pile, void *area);

void cake_allocator_init();

#endif //SEER_CAKE_PILE_H
