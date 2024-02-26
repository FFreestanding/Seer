#include <gdt/gdt.h>
#include <gdt/sd_high32.h>
#include <gdt/sd_low32.h>
#include <stdint.h>

//第一个保留项，不使用
#define GDT_ENTRY_NUMS 5
uint64_t _gdt[GDT_ENTRY_NUMS];
uint16_t _gdt_limit = sizeof(_gdt) - 1;

void
_set_gdt_entry
(uint32_t index, uint32_t base, uint32_t limit, uint32_t flags)
{
    if (index>=5)
    {
        return;
    }
    _gdt[index] = SEGMENT_BASE_ADDRESS_31_24(base) | flags |
                        SEGMENT_LIMIT_19_16(limit) | SEGMENT_BASE_ADDRESS_23_16(base);
    _gdt[index] <<= 32;
    _gdt[index] |= SEGMENT_BASE_ADDRESS_15_0(base) | SEGMENT_LIMIT_15_0(limit);
}



void
_init_gdt()
{
    // Flat Memory
    _set_gdt_entry(0, 0, 0, 0);
    _set_gdt_entry(1, 0, 0xfffff, SEGMENT_CODE_R0);
    _set_gdt_entry(2, 0, 0xfffff, SEGMENT_DATA_R0);//stack
    _set_gdt_entry(3, 0, 0xfffff, SEGMENT_CODE_R3);
    _set_gdt_entry(4, 0, 0xfffff, SEGMENT_DATA_R3);
}