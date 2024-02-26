#ifndef __GDT_H
#define __GDT_H 1
#include <stdint.h>

void
_set_gdt_entry
(uint32_t, uint32_t, uint32_t, uint32_t);

void
_init_gdt();

#endif