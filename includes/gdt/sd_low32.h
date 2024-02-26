#ifndef __SD_LOW32_H
#define __SD_LOW32_H 1

/*--- Structure of Segment Descriptor low 32 ---*/

//16-31 Base 15:00
#define SEGMENT_BASE_ADDRESS_15_0(x)    ((x & 0x0000ffff) << 16)
//0-15 Segment Limit 15:00(15-0)
#define SEGMENT_LIMIT_15_0(x)           (x & 0xffff)

#endif