#ifndef __IDT_H
#define __IDT_H 1

/*
    故障 fault
    自陷 trap
    终止 abort
*/
/*
    Ref: Intel Manuel Vol.3 
    6.3 SOURCES OF INTERRUPTS 
        Table 6-1. Exceptions and Interrupts
    6.15 EXCEPTION AND INTERRUPT REFERENCE
*/

// Divide Error, FAULT
#define INTERRUPT_DIVISION_EXCEPTION             0
// Debug, TRAP OR FAULT
#define INTERRUPT_DEBUG_EXCEPTION                1
// NMI Interrupt, Not applicable
#define INTERRUPT_NMI                            2
// Breakpoint, TRAP
#define INTERRUPT_BREAKPOINT                     3
// Overflow, TRAP
#define INTERRUPT_OVERFLOW                       4
// BOUND Range Exceeded, FAULT
#define INTERRUPT_BOUND_RANGE_EXCEEDED           5
// Invalid Opcode (Undefined Opcode), FAULT
#define INTERRUPT_INVALID_OPCODE                 6
// Device Not Available (No Math Coprocessor), FAULT
#define INTERRUPT_DEVICE_NOT_AVAILABLE           7
// Double Fault, ABORT
#define INTERRUPT_DOUBLE_FAULT                   8
// CoProcessor Segment Overrun (reserved), ABORT
#define INTERRUPT_COPROCESSOR_SEGMENT_OVERRUN    9
// Invalid TSS, FAULT
#define INTERRUPT_INVALID_TSS                   10
// Segment Not Present, FAULT
#define INTERRUPT_SEGMENT_NOT_PRESENT           11
// Stack Segment Fault, FAULT
#define INTERRUPT_STACK_SEGMENT_FAULT           12
// General Protection, FAULT
#define INTERRUPT_GENERAL_PROTECTION            13
// Page Fault, FAULT
#define INTERRUPT_PAGE_FAULT                    14
// Reserved, ???
#define INTERRUPT_RESERVED                      15
// Floating-Point Error (Math Fault), FAULT
#define INTERRUPT_FLOATING_POINT_ERROR          16
// Alignment Check, FAULT
#define INTERRUPT_ALIGNMENT_CHECK               17
// Machine Check, ABORT
#define INTERRUPT_MACHINE_CHECK                 18
// SIMD Floating-Point Exception, FAULT
#define INTERRUPT_SIMD_FLOATING_POINT_EXCEPTION 19
// Virtualization Exception, FAULT
#define INTERRUPT_VIRTUALIZATION_EXCEPTION      20
// Control Protection Exception, FAULT
#define INTERRUPT_CONTROL_PROTECTION_EXCEPTION  21
// LunaixOS related
#define LUNAIX_SYS_PANIC                32

#ifndef __ASM__
#include <idt/interrupts.h>

void
_init_idt();

void
isr0(isr_param*);

void
isr1(isr_param*);

void
isr2(isr_param*);

void
isr3(isr_param*);

void
isr4(isr_param*);

void
isr5(isr_param*);

void
isr6(isr_param*);

void
isr7(isr_param*);

void
isr8(isr_param*);

void
isr9(isr_param*);

void
isr10(isr_param*);

void
isr11(isr_param*);

void
isr12(isr_param*);

void
isr13(isr_param*);

void
isr14(isr_param*);

void
isr15(isr_param*);

void
isr16(isr_param*);

void
isr17(isr_param*);

void
isr18(isr_param*);

void
isr19(isr_param*);

void
isr20(isr_param*);

void
isr21(isr_param*);

void
isr250(isr_param*);

void
isr251(isr_param*);

void
isr252(isr_param*);

void
isr253(isr_param*);

void
isr201(isr_param*);

void
isr210(isr_param*);

#endif

#endif