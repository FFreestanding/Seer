#ifndef _APIC_H_
#define _APIC_H_

#include <apic/cpu.h>
#include <apic/lvt.h>

#define REDIRECT_ERROR_VECTOR                   250
#define REDIRECT_LINT0_VECTOR                   251
#define REDIRECT_SPIV_VECTOR                    252
#define REDIRECT_APIC_TIMER_VECTOR              253

/*
    Intel Manual Vol. 3A 10-6
    Table 10-1. Local APIC Register Address Map
*/
#define APIC_IDR        0x20        // ID Reg
#define APIC_VER        0x30        // Version Reg
#define APIC_TPR        0x80        // Task Priority
#define APIC_APR        0x90        // Arbitration Priority
#define APIC_PPR        0xA0        // Processor Priority
#define APIC_EOI        0xB0        // End-Of-Interrupt
#define APIC_RRD        0xC0        // Remote Read
#define APIC_LDR        0xD0        // Local Destination Reg
#define APIC_DFR        0xE0        // Destination Format Reg
#define APIC_SPIVR      0xF0        // Spurious Interrupt Vector Reg
#define APIC_ISR_BASE   0x100       // Base address for In-Service-Interrupt bitmap register (256bits)
#define APIC_TMR_BASE   0x180       // Base address for Trigger-Mode bitmap register (256bits)
#define APIC_IRR_BASE   0x200       // Base address for Interrupt-Request bitmap register (256bits)
#define APIC_ESR        0x280       // Error Status Reg
#define APIC_ICR_BASE   0x300       // Interrupt Command
#define APIC_LVT_LINT0  0x350
#define APIC_LVT_LINT1  0x360
#define APIC_LVT_ERROR  0x370

#define APIC_BASE_VIRTUAL_ADDRESS 0x210000
#define APIC_BASE_PHYSICAL_ADDRESS 0xFEE00000

#define IA32_MSR_APIC_BASE 0x1B
#define IA32_APIC_ENABLE   0x800

// Table 10-6. Local APIC Register Address Map Supported by x2APIC (Contd.)
#define APIC_TIMER_LVT  0x320
// Figure 10-11. Initial Count and Current Count Registers
#define APIC_TIMER_ICR 0x380
#define APIC_TIMER_CCR  0x390       // Current Count
#define APIC_TIMER_DCR  0x3E0       // Divide Configuration

#define CONSTANT_ICR 0x100000

// Dividers for timer. See Intel Manual Vol3A. 10-17 (pp. 3207), Figure 10-10
#define APIC_TIMER_DIV64            0b1001
#define APIC_TIMER_DIV128           0b1010

#define _apic_read_register(offset) (*(uint32_t*)(APIC_BASE_VIRTUAL_ADDRESS + (offset)))
#define _apic_write_register(offset, value)     (*(uint32_t*)(APIC_BASE_VIRTUAL_ADDRESS + (offset)) = (value))
#define _apic_setup_lvts() _apic_write_register(APIC_LVT_LINT0, LVT_ENTRY_LINT0_DEFAULT(REDIRECT_LINT0_VECTOR));\
                            _apic_write_register(APIC_LVT_LINT1, LVT_ENTRY_LINT1_DEFAULT);\
                            _apic_write_register(APIC_LVT_ERROR, LVT_ENTRY_ERROR_DEFAULT(REDIRECT_ERROR_VECTOR));

uint8_t
_apic_init();

inline static void
apic_done_servicing();

inline static void
apic_done_servicing()
{
    _apic_write_register(APIC_EOI, 0);
}

#endif // _APIC_H_