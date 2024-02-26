#ifndef __INTERRUPTS_H
#define __INTERRUPTS_H 1
#include <stdint.h>
#include <apic/cpu.h>

#pragma pack(1)
typedef struct {
    gp_regs registers;
    uint32_t vector;
    uint32_t err_code;
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
    uint32_t esp;
    uint32_t ss;
} isr_param;
#pragma pack()

typedef struct
{
    void (*routines[256])(isr_param*);
} interrupt_routines_manager;

interrupt_routines_manager*
get_interrupt_routines_mananger_instance();

void
interrupt_handler(isr_param* param);

void
_interrupt_routine_init();

void 
interrupt_routine_subscribe
(const uint8_t vector, void(*p)(isr_param*));

void 
interrupt_routine_unsubscribe
(const uint8_t vector);

void
_asm_isr0();

void
_asm_isr1();

void
_asm_isr2();

void
_asm_isr3();

void
_asm_isr4();

void
_asm_isr5();

void
_asm_isr6();

void
_asm_isr7();

void
_asm_isr8();

void
_asm_isr9();

void
_asm_isr10();

void
_asm_isr11();

void
_asm_isr12();

void
_asm_isr13();

void
_asm_isr14();

void
_asm_isr15();

void
_asm_isr16();

void
_asm_isr17();

void
_asm_isr18();

void
_asm_isr19();

void
_asm_isr20();

void
_asm_isr21();

void
_asm_isr33();

void
_asm_isr250();

void
_asm_isr251();

void
_asm_isr252();

void
_asm_isr253();

void
_asm_isr210();

void
_asm_isr201();

#endif