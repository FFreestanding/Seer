#ifndef __SD_HIGH32_H
#define __SD_HIGH32_H 1

/*--- Structure of Segment Descriptor high 32 ---*/
//BASE - Segment base address

//0-7 Base 23:16(7-0)
#define SEGMENT_BASE_ADDRESS_23_16(x)   ((x & 0x00ff0000) >> 16)
//8-11 TYPE - Segment type
#define SEGMENT_TYPE(x)                 ((x & 0xf) << 8)
//12 S - Descriptor type (0 = system; 1 = code or data)
#define DESCRIPTOR_TYPE(x)              ((x & 0b1) << 12)
//13-14 DPL - Descriptor prilege level
#define DESCRIPTOR_PRILEGE_LEVEL(x)     ((x & 0b11) << 13)
//15 P - Segment present （1：不用虚拟内存）
#define SEGMENT_PRESENT(x)              ((x & 0b11) << 15)
//16-19 LIMIT - Segment Limit 19:16(19-16)
#define SEGMENT_LIMIT_19_16(x)          ((x & 0xf) << 16)
//20 AVL - Available for use by system software
#define AVAILABLE_FOR_USE(x)            ((x & 0b1) << 20)
//21 L - 64-bit code segment (IA-32e mode only)
#define BIT64_CODE_SEGMENT(x)           ((x & 0b1) << 21)
//22 D/B - Default operation size (0 = 16-bit segment; 1 = 32-bit segment)
#define DEFAULT_OPERATION_SIZE(x)       ((x & 0b1) << 22)
//23 G - Granularity (1：4K对齐)
#define GRANULARITY(x)                  ((x & 0b1) << 23)
//24-31 Base 31:24(31-24)
#define SEGMENT_BASE_ADDRESS_31_24(x)   (x & 0xff000000)

/*--- Code and Data Segment Types ---*/

// Data Read-Only
#define SEGMENT_DATA_R          0b0000
// Data Read-Only, accessed
#define SEGMENT_DATA_R_A        0b0001
// Data Read/Write
#define SEGMENT_DATA_R_W        0b0010
// Data Read/Write, accessed
#define SEGMENT_DATA_R_W_A      0b0011
// Data Read-Only, expand-down
#define SEGMENT_DATA_R_E        0b0100
// Data Read-Only, expand-down, accessed
#define SEGMENT_DATA_R_E_A      0b0101
// Data Read/Write, expand-down
#define SEGMENT_DATA_R_W_E      0b0110
// Data Read/Write, expand-down, accessed
#define SEGMENT_DATA_R_W_E_A    0b0111

// Code Execute-Only
#define SEGMENT_CODE_E          0b1000
// Code Execute-Only, accessed
#define SEGMENT_CODE_E_A        0b1001
// Code Execute/Read
#define SEGMENT_CODE_E_R        0b1010
// Code Execute/Read, accessed
#define SEGMENT_CODE_E_R_A      0b1011
// Code Execute-Only, conforming
#define SEGMENT_CODE_E_C        0b1100
// Code  Execute-Only, conforming, accessed
#define SEGMENT_CODE_E_C_A      0b1101
// Code Execute/Read, conforming
#define SEGMENT_CODE_E_R_C      0b1110
// Code Execute/Read, conforming, accessed
#define SEGMENT_CODE_E_R_C_A    0b1111

#define SEGMENT_DATA_R0         SEGMENT_TYPE(SEGMENT_DATA_R_W) | DESCRIPTOR_TYPE(1) |\
                                DESCRIPTOR_PRILEGE_LEVEL(0) | SEGMENT_PRESENT(1) |\
                                AVAILABLE_FOR_USE(0) | BIT64_CODE_SEGMENT(0) |\
                                DEFAULT_OPERATION_SIZE(1) | GRANULARITY(1)

#define SEGMENT_DATA_R3         SEGMENT_TYPE(SEGMENT_DATA_R_W) | DESCRIPTOR_TYPE(1) |\
                                DESCRIPTOR_PRILEGE_LEVEL(3) | SEGMENT_PRESENT(1) |\
                                AVAILABLE_FOR_USE(0) | BIT64_CODE_SEGMENT(0) |\
                                DEFAULT_OPERATION_SIZE(1) | GRANULARITY(1)

#define SEGMENT_CODE_R0         SEGMENT_TYPE(SEGMENT_CODE_E_R) | DESCRIPTOR_TYPE(1) |\
                                DESCRIPTOR_PRILEGE_LEVEL(0) | SEGMENT_PRESENT(1) |\
                                AVAILABLE_FOR_USE(0) | BIT64_CODE_SEGMENT(0) |\
                                DEFAULT_OPERATION_SIZE(1) | GRANULARITY(1)

#define SEGMENT_CODE_R3         SEGMENT_TYPE(SEGMENT_CODE_E_R) | DESCRIPTOR_TYPE(1) |\
                                DESCRIPTOR_PRILEGE_LEVEL(3) | SEGMENT_PRESENT(1) |\
                                AVAILABLE_FOR_USE(0) | BIT64_CODE_SEGMENT(0) |\
                                DEFAULT_OPERATION_SIZE(1) | GRANULARITY(1)
  

#endif