#ifndef _IOAPIC_H_
#define _IOAPIC_H_
#include <stdint.h>
#include <apic/acpi.h>
#define IOAPIC_BASE_VIRTUAL_ADDRESS 0x220000

// https://pdos.csail.mit.edu/6.828/2018/readings/ia32/ioapic.pdf

/* 3.2.1. IOAPICID—IOAPIC IDENTIFICATION REGISTER */
#define IOAPICID 0x00
// 31:28 Reserved.
// 27:24 IOAPIC Identification—R/W. This 4 bit field contains the IOAPIC identification.
// 23:0 Reserved.

/* 3.2.2. IOAPICVER—IOAPIC VERSION REGISTER */
#define IOAPICVER_OFFSET 0x01
// 31:24 Reserved.
/* 23:16 Maximum Redirection Entry—RO. This field contains the entry number (0 being the lowest
entry) of the highest entry in the I/O Redirection Table. The value is equal to the number of
interrupt input pins for the IOAPIC minus one. The range of values is 0 through 239. For this
IOAPIC, the value is 17h. */
// 15:8 Reserved.
/* 7:0 APIC VERSION—RO. This 8 bit field identifies the implementation version. The version number
assigned to the IOAPIC is 11h */

/* 3.2.3. IOAPICARB—IOAPIC ARBITRATION REGISTER */
#define IOAPICARB_OFFSET 0x2
// 31:28 Reserved.
// 27:24 IOAPIC Identification—R/W. This 4 bit field contains the IOAPIC Arbitration ID.
// 23:0 Reserved.

/* 2.1. System Bus Signals : D/I# */

#define IOAPIC_IOREGSEL 0x00
#define IOAPIC_IOWIN    0x10

/* 3.2.4. IOREDTBL[23:0]—I/O REDIRECTION TABLE REGISTERS */
#define IOAPIC_IOREDTBL_OFFSET 0x10

/* Table of 3.2.4 */

// 63-56 Destination Field—R/W
#define IOAPIC_DESTINAMTION_FIELD(x) ((x)&0xff<<(56-32))
// 55-17 Reserved

// 16 Interrupt Mask—R/W

// 15 Trigger Mode—R/W

// 14 Remote IRR—RO

// 13 Interrupt Input Pin Polarity (INTPOL)—R/W

// 12 Delivery Status (DELIVS)—RO

// 11 Destination Mode (DESTMOD)—R/W
#define IOAPIC_DESTINATION_MOD(x) (((x)&0b1)<<11)
// 10-8 Delivery Mode (DELMOD)—R/W
#define IOAPIC_DELIVERY_MODE(x) (((x)&0b111)<<8)
#define IOAPIC_DELIVERY_MODE_FIXED            0b000
#define IOAPIC_DELIVERY_MODE_LOWEST_PRIORITY  0b001
#define IOAPIC_DELIVERY_MODE_NMI              0b100
// 7-0 Interrupt Vector (INTVEC)—R/W
#define IOAPIC_INTERRUPT_VECTOR(x) ((x)&0xff)


/* 3.1.1. IOREGSEL—I/O REGISTER SELECT REGISTER
This register selects the IOAPIC Register to be read/written. The data is then read from or written to the selected
register through the IOWIN Register. */

// 31:8 Reserved.

/* 7:0 APIC Register Address—R/W. Bits [7:0] specify the IOAPIC register to be read/written via the
IOWIN Register. */
#define IOAPIC_IOREGSEL_ADDRESS(x) ((x)&0xff)


/* 3.1.2. IOWIN—I/O WINDOW REGISTER
This register is used to write to and read from the register selected by the IOREGSEL Register.
Readability/writability is determined by the IOAPIC register that is currently selected.
Bit Description */
/* 31:0 APIC Register Data—R/W. Memory references to this register are mapped to the APIC register
specified by the contents of the IOREGSEL Register. */
#define IOAPIC_IOWIN_REGISTER(x) ((x)&0xffffffff)


void _ioapic_init();

uint8_t
ioapic_get_irq(acpi_context* acpi_ctx, uint8_t old_irq);

// Table 1. Memory Mapped Registers For Accessing IOAPIC Registers
void
ioapic_write(uint8_t offset, uint32_t val);

uint32_t
ioapic_read(uint8_t offset);
void
ioapic_redirect(uint8_t irq, uint8_t vector, uint8_t dest, uint32_t flags);

#endif // _IOAPIC_H_