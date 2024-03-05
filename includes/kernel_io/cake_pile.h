#ifndef SEER_CAKE_PILE_H
#define SEER_CAKE_PILE_H

#include <data_structure/llist.h>


struct cake_piece {

};

struct cake {
    struct llist_header other_cakes;
    void *first_piece;
    unsigned int used_pieces;
    unsigned int next_free;
    uint32_t free_list[0];
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
    char pile_name[32];
};

void cake_pile_init();

void new_cake_piece();

void new_cake();

void new_cake_pile();

#endif //SEER_CAKE_PILE_H
