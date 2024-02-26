#ifndef _LVT_H_
#define _LVT_H_

/*
    Local Vector Table (LVT)
    The local vector table (LVT) allows software to specify the manner in which the local interrupts are delivered to the
    processor core. It consists of the following 32-bit APIC registers (see Figure 10-8)
    Vol. 3A 10-13 
    Figure 10-8. 
*/

// 0-7 Vector
#define LVT_VECTOR(x) ((x)&0xff)
/* 8-10 Delivery Mode
000: Fixed
010: SMI
100: NMI
111: ExtlNT
101: INIT
All other combinations are reserved */
#define LVT_DELIVERY_MODE(x) ((x&0b111)<<8)
/* 12 Delivery Status
0: Idle
1: Send Pending */
#define LVT_DELIVERY_STATUS(x) ((x&0b1)<<12)
/* 13 Interrupt Input Pin Polarity
Specifies the polarity of the corresponding interrupt pin: (0) active high or (1) active low. */
#define LVT_INTERRUPT_INPUT_PIN_POLARITY(x) ((x&0b1)<<13)
/* 14 Remote IRR Flag (Read Only)
For fixed mode, level-triggered interrupts; this flag is set when the local APIC accepts the
interrupt for servicing and is reset when an EOI command is received from the processor. The
meaning of this flag is undefined for edge-triggered interrupts and other delivery modes. */
// #define LVT_REMOTE_IRR_FLAG(x) 

/* 15 Trigger Mode 
Selects the trigger mode for the local LINT0 and LINT1 pins: (0) edge sensitive and (1) level
sensitive. This flag is only used when the delivery mode is Fixed. When the delivery mode is
NMI, SMI, or INIT, the trigger mode is always edge sensitive. When the delivery mode is
ExtINT, the trigger mode is always level sensitive. The timer and error interrupts are always
treated as edge sensitive.
If the local APIC is not used in conjunction with an I/O APIC and fixed delivery mode is
selected; the Pentium 4, Intel Xeon, and P6 family processors will always use level-sensitive
triggering, regardless if edge-sensitive triggering is selected.
Software should always set the trigger mode in the LVT LINT1 register to 0 (edge sensitive).
Level-sensitive interrupts are not supported for LINT1. */
#define LVT_TRIGGER_MODE(x) ((x&0b1)<<15)

/* 16 Mask 
Interrupt mask: (0) enables reception of the interrupt and (1) inhibits reception of the interrupt. 
When the local APIC handles a performance-monitoring counters interrupt, 
it automatically sets the mask flag in the LVT performance counter register. This flag is set to 1 on reset.
It can be cleared only by software.*/
#define LVT_MASK(x) ((x&0b1)<<16)

/* 17-18 Timer Mode 
Bits 18:17 selects the timer mode (see Section 10.5.4):
(00b) one-shot mode using a count-down value,
(01b) periodic mode reloading a count-down value,
(10b) TSC-Deadline mode using absolute target value in IA32_TSC_DEADLINE MSR (see
Section 10.5.4.1),
(11b) is reserved. */
#define LVT_TIMER_MODE(x) ((x&0b11)<<17)


// #define LVT_DELIVERY_FIXED          0
// #define LVT_DELIVERY_NMI            (0x4 << 8)
// #define LVT_TRIGGER_EDGE            (0   << 15)
// #define LVT_TRIGGER_LEVEL           (1   << 15)
// #define LVT_MASKED                  (1   << 16)
// #define LVT_TIMER_ONESHOT           (0   << 17)
// #define LVT_TIMER_PERIODIC          (1   << 17)

#define LVT_ENTRY_LINT0_DEFAULT(vector)           (LVT_DELIVERY_MODE(0b000) | (vector))
// Pin LINT#1 is configured for relaying NMI, but we masked it here as I think
//  it is too early for that
// LINT#1 *must* be edge trigged (Intel manual vol3. 10-14)
#define LVT_ENTRY_LINT1_DEFAULT                   (LVT_DELIVERY_MODE(0b100) | LVT_MASK(0b1) | LVT_TRIGGER_MODE(0b0))
#define LVT_ENTRY_ERROR_DEFAULT(vector)           (LVT_DELIVERY_MODE(0b000) | (vector))
#define LVT_TIMER_DEFAULT(vector)                 (LVT_TIMER_MODE(0b00) | (vector) | LVT_DELIVERY_MODE(0b000))

#endif // _LVT_H_