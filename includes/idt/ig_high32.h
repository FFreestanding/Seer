#ifndef __IG_HIGH32_H
#define __IG_HIGH32_H 1
#include <stdint.h>

/*--- Structure of Interrupt Gate high 32 ---*/

//0-7 Reserved
#define INTERRUPT_GATE_REVERSED(x)                 (x & 0xf)
/*8-11 Gate Type: A 4-bit value which defines the type of gate this Interrupt Descriptor represents. There are five valid type values:
0b0101 or 0x5: Task Gate, note that in this case, the Offset value is unused and should be set to zero.
0b0110 or 0x6: 16-bit Interrupt Gate
0b0111 or 0x7: 16-bit Trap Gate
0b1110 or 0xE: 32-bit Interrupt Gate
0b1111 or 0xF: 32-bit Trap Gate*/
#define INTERRUPT_GATE_TYPE(x)                      ((x & 0xf) << 8)
//12 default 0
#define INTERRUPT_GATE_12                      (0b0 << 12)
/*13-14 DPL: A 2-bit value which defines the CPU Privilege Levels which are allowed to access this interrupt via the INT instruction.
Hardware interrupts ignore this mechanism.*/
#define INTERRUPT_GATE_DPL(x)                       ((x & 0b11) << 13)
//15 第 15 位（S 标志）用于指示中断门的类型。当 S 标志为 0 时，表示该中断门是一个中断门（Interrupt Gate），用于处理外部中断。当 S 标志为 1 时，表示该中断门是一个陷阱门（Trap Gate），用于处理陷阱（例如软中断）。
#define INTERRUPT_GATE_15(x)                        ((x & 0b1) << 15)
/*16-31 Offset 31:16(31-16)
A 32-bit value, split in two parts. It represents the address of the entry point of the Interrupt Service Routine.*/
#define INTERRUPT_GATE_ISR_OFFSET_31_16(x)          ((uint32_t)(x) & 0xffff0000)

#define INTERRUPT_GATE_32_BITS 0b1110

#define INTERRUPT_GATE_RING0   0b000

#define DEFAULT_IDT_ATTR  (INTERRUPT_GATE_REVERSED(0) | INTERRUPT_GATE_TYPE(INTERRUPT_GATE_32_BITS) | INTERRUPT_GATE_12 | \
                            INTERRUPT_GATE_DPL(INTERRUPT_GATE_RING0) | INTERRUPT_GATE_15(1))

#endif