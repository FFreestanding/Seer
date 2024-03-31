#include <apic/ioapic.h>
#include <apic/acpi.h>
#include <kernel_io/memory.h>
#include <apic/rtc.h>
#include "common.h"

void _ioapic_init()
{
    // Remapping the IRQs
    
    acpi_context* acpi_ctx = get_acpi_context_instance();

    uint8_t irq_rtc = 0;
    ASSERT(irq_rtc = ioapic_get_irq(acpi_ctx, RTC_IRQ), "io apic redirect error (equal 0)");
    kernel_log(INFO, "irq rtc %u", irq_rtc);
    ioapic_redirect(irq_rtc, REDIRECT_RTC_TIMER_VECTOR, 0, IOAPIC_DELIVERY_MODE(IOAPIC_DELIVERY_MODE_FIXED));
}

uint8_t
ioapic_get_irq(acpi_context* acpi_ctx, uint8_t old_irq)
{
    if (old_irq>=24)
    {
        return old_irq;
    }
    uint8_t ret = (acpi_isos_t*)acpi_ctx->irq[old_irq];
    return ret?ret:old_irq;
}

/* To reference an IOAPIC register, 
a byte memory write that the PIIX3 decodes for the IOAPIC loads the IOREGSEL Register with an 8-bit value 
that specifies the IOAPIC register (address offset in Table 3.2) to be accessed. 
The IOWIN Register is then used to read/write the desired data from/to the IOAPIC register 
specified by bits [7:0] of the IOREGSEL Register. 
The IOWIN Register must be accessed as a Dword quantity. */
void 
ioapic_write(uint8_t offset, uint32_t value)
{
    *((volatile uint32_t*)(IOAPIC_BASE_VIRTUAL_ADDRESS + IOAPIC_IOREGSEL)) = offset;
    *((volatile uint32_t*)(IOAPIC_BASE_VIRTUAL_ADDRESS + IOAPIC_IOWIN)) = value;
}

uint32_t
ioapic_read(uint8_t offset)
{
    *((volatile uint32_t*)(IOAPIC_BASE_VIRTUAL_ADDRESS + IOAPIC_IOREGSEL)) = offset;
    return *((volatile uint32_t*)(IOAPIC_BASE_VIRTUAL_ADDRESS + IOAPIC_IOWIN));
}

/* 3.2.4. IOREDTBL[23:0]—I/O REDIRECTION TABLE REGISTERS */
void
ioapic_redirect(uint8_t irq, uint8_t vector, uint8_t dest, uint32_t flags)
{
    uint8_t reg_sel = IOAPIC_IOREDTBL_OFFSET + irq * 2;
    // Write low 32 bits
    ioapic_write(reg_sel, IOAPIC_INTERRUPT_VECTOR(vector)|flags);
    // Write high 32 bits
    ioapic_write(reg_sel + 1, IOAPIC_DESTINAMTION_FIELD(dest));
}