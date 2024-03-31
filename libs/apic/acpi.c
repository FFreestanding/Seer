#include <apic/acpi.h>
#include <stdint.h>
#include <kernel_io/memory.h>
#include <kernel_io/valloc.h>
#include "common.h"

void 
_acpi_init(multiboot_info_t* mb_info)
{
    acpi_rsdp_t* rsdp = acpi_locate_rsdp(mb_info);

    ASSERT(rsdp && acpi_rsdp_validate(rsdp), "RSDP address invalid");

    kernel_log(INFO, "RSDP found at %h, RSDT: %h", rsdp, rsdp->rsdt);
    acpi_context* ctx = get_acpi_context_instance();
    acpi_rsdt_t* rsdt = rsdp->rsdt;

    memory_copy(rsdt->header.oem_id, ctx->oem_id, 6);
    ctx->oem_id[6] = '\0';
    uint32_t entry_num = (rsdt->header.length - sizeof(acpi_header_t)) >> 2;
    for (uint32_t i = 0; i < entry_num; i++)
    {
        acpi_header_t* entry = ((acpi_lapic_t**)&(rsdt->entry))[i];
        switch (entry->signature) 
        {
            // https://uefi.org/htmlspecs/ACPI_Spec_6_4_html/05_ACPI_Software_Programming_Model/ACPI_Software_Programming_Model.html#interrupt-controller-structure-types
            case ACPI_MADT_SIG:
                madt_parse((acpi_madt_t*)entry, ctx);
                break;
            case ACPI_FADT_SIG:
                ctx->fadt = *(acpi_fadt_t*)entry;
                break;
            default:
                break;
        }
    }
    kernel_log(INFO, "OEM: %s", ctx->oem_id);
    kernel_log(INFO, "IOAPIC address: %h", ctx->ioapic->ioapic_address);
    kernel_log(INFO, "LAPIC address: %h", ctx->lapic_address);

    for (uint32_t i = 0; i < 24; i++)
    {
        acpi_isos_t* intso = ctx->irq[i];
        if (!intso) { continue; }
        kernel_log(INFO, "IRQ #%u -> GSI #%u", intso->source, intso->global_system_interrupt);
    }
}


acpi_rsdp_t*
acpi_locate_rsdp(multiboot_info_t* mb_info)
{
    acpi_rsdp_t* rsdp = 0;
    // use 0x4000 to bypass something
    // why mem_start += 16 ?
    // OSPM finds the Root System Description Pointer (RSDP) structure by searching physical memory ranges on 16-byte boundaries
    // https://uefi.org/htmlspecs/ACPI_Spec_6_4_html/05_ACPI_Software_Programming_Model/ACPI_Software_Programming_Model.html#finding-the-rsdp-on-ia-pc-systems
    for (uint8_t* mem_start = (uint8_t*)0x4000; mem_start < 0x100000; mem_start += 16)
    {
        if (*((uint32_t*)(mem_start)) == ACPI_RSDP_SIG_L)
        {
            rsdp = (acpi_rsdp_t*)(mem_start);
            return rsdp;
        }
    }

    return rsdp;
}

uint8_t
acpi_rsdp_validate(acpi_rsdp_t* rsdp)
{
    uint8_t sum = 0;
    uint8_t* rsdp_ptr = (uint8_t*)rsdp;
    for (uint32_t i = 0; i < 20; i++) {
        sum += *(rsdp_ptr + i);
    }
    return sum == 0;
}

acpi_context*
get_acpi_context_instance()
{
    static acpi_context acpi_ctx;
    return &acpi_ctx;
}

void
madt_parse(acpi_madt_t* madt, acpi_context* toc)
{
    toc->lapic_address = madt->local_interrupt_controller_address;
    
    uint8_t* ics_start = (uint8_t*)((uintptr_t)madt + sizeof(acpi_madt_t));
    uintptr_t ics_end = (uintptr_t)madt + madt->header.length;

    toc->irq = (acpi_isos_t*)valloc(24*sizeof(acpi_isos_t*));
    memory_set_fast(toc->irq, 0, 24*sizeof(acpi_isos_t*)/4);
    uint32_t so_idx = 0;
    while (ics_start < ics_end)
    {
        acpi_interrupt_controller_struct* entry = (acpi_interrupt_controller_struct*)ics_start;
        switch (entry->type)
        {
            case ACPI_MADT_LAPIC:
            {
                toc->lapic = (acpi_lapic_t*)entry;
                break;
            }
            case ACPI_MADT_IOAPIC:
            {
                toc->ioapic = (acpi_ioapic_t*)entry;
                break;
            }
            case ACPI_MADT_INTSO:
            {
                acpi_isos_t* intso_tbl = (acpi_isos_t*)entry;
                toc->irq[intso_tbl->source] = intso_tbl;
                break;
            }
            default:
                break;
        }
        ics_start += entry->length;

    }
}