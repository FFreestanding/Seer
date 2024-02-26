#ifndef __IG_LOW32_H
#define __IG_LOW32_H 1

/*--- Structure of Interrupt Gate low 32 ---*/

/*0-15 Offset 0:15(0-15)
A 32-bit value, split in two parts. It represents the address of the entry point of the Interrupt Service Routine.*/
#define INTERRUPT_GATE_ISR_OFFSET_0_15(x)   (x & 0x0000ffff)
//16-31 Segment Selector
#define INTERRUPT_GATE_SEGMENT_SELECTOR(x)                 ((x & 0xffff) << 16)

#define SEGMENT_SELECTOR_CODE_R0 0b0000000000001000
#define SEGMENT_SELECTOR_DATA_R0 0b0000000000010000
#define SEGMENT_SELECTOR_CODE_R3 0b0000000000011000
#define SEGMENT_SELECTOR_DATA_R3 0b0000000000100000

#endif