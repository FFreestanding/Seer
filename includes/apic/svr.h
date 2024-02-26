#ifndef _SVR_H_
#define _SVR_H_
#include <apic/lvt.h>
/*
    Spurious-Interrupt Vector Register (SVR)
    10-34 Vol. 3A 
    Figure 10-23. 
*/

// 0-7 Spurious Vector - redirect spurious interrupt to another vector number
#define SPURIOUS_VECTOR(x) (x&0xff)
/* 8 APIC Software Enable/Disable
0: APIC Disabled
1: APIC Enabled */
#define APIC_SOFTWARE_ENABLE(x) ((x&0b1)<<8)

/* 9 Focus Processor Checking
0: Disabled
1: Enabled */
#define FOCUS_PROCESSOR_CHECKING(x) ((x&0b1)<<9)

/* 12 EOI-Broadcast Suppression
0: Disabled
1: Enabled */
#define EOI_BROADCAST_SUPPRESSION(x) ((x&0b1)<<12)

#define SVR_DEFAULT(source_flags) source_flags | APIC_SOFTWARE_ENABLE(1) | \
                                        SPURIOUS_VECTOR(REDIRECT_SPIV_VECTOR)


#endif // _SVR_H_