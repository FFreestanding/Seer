/**
 * @file llist.h
 * @author Lunaixsky
 * @brief This doubly linked cyclic list is adopted from Linux kernel <linux/list.h>
 * @version 0.1
 * @date 2022-03-12
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#ifndef __LUNAIX_LLIST_H
#define __LUNAIX_LLIST_H

#include <stdint.h>
#include <stddef.h>

struct llist_header
{
    struct llist_header* prev;
    struct llist_header* next;
};

static inline void
__llist_add(struct llist_header* elem,
            struct llist_header* prev,
            struct llist_header* next)
{
    next->prev = elem;
    elem->next = next;
    elem->prev = prev;
    prev->next = elem;
}

static inline void
llist_init_head(struct llist_header* head) {
    head->next = head;
    head->prev = head;
}

static inline void
llist_append(struct llist_header* head, struct llist_header* elem)
{
    __llist_add(elem, head, head->next);
}

static inline void
llist_prepend(struct llist_header* head, struct llist_header* elem)
{
    __llist_add(elem, head->prev, head);
}

static inline void
llist_delete(struct llist_header* elem) {
    elem->prev->next = elem->next;
    elem->next->prev = elem->next;
    
    // make elem orphaned
    elem->prev = elem;
    elem->next = elem;
}

/**
 * list_entry - get the struct for this entry
 * @ptr:	the &struct list_head pointer.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_struct within the struct.
 */
#define list_entry(ptr, type, member) \
	container_of(ptr, type, member)

/**
 * list_for_each_entry	-	iterate over list of given type
 * @pos:	the type * to use as a loop counter.
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 * @link: https://blog.csdn.net/u012503639/article/details/77771814
 */
#define llist_for_each(pos, n, head, member)				    \
	for (pos = list_entry((head)->next, typeof(*pos), member),	\
		n = list_entry(pos->member.next, typeof(*pos), member);	\
	     &pos->member != (head); 					            \
	     pos = n, n = list_entry(n->member.next, typeof(*n), member))

// From Linux kernel v2.6.0 <kernel.h:194>
/**
 * container_of - cast a member of a structure out to the containing structure
 *
 * @ptr:	the pointer to the member.
 * @type:	the type of the container struct this is embedded in.
 * @member:	the name of the member within the struct.
 *
 */
#define container_of(ptr, type, member) ({			\
        const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
        (type *)( (char *)__mptr - offsetof(type,member) );})

static inline int
llist_empty(struct llist_header* elem)
{
    return elem->next == elem && elem->prev == elem;
}

#endif /* __LUNAIX_LLIST_H */
