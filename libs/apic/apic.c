#include <apic/apic.h>
#include <kernel_io/memory.h>
#include <apic/tpp.h>
#include <apic/svr.h>

uint8_t _apic_init()
{
    disable_interrupts();

    // ensure that the APIC exits
    _assert(_cpu_has_apic(), "No APIC detected!")

    // disable 8259 PIC
    // ref: https://wiki.osdev.org/8259_PIC
    asm volatile (
        "movb $0xff, %al\n"
        "outb %al, $0xa1\n"
        "outb %al, $0x21\n"
    );

    // (hardware) enable LAPIC
    asm volatile (
        "movl %0, %%ecx\n"
        "rdmsr\n"
        "orl %1, %%eax\n"
        "wrmsr\n"
        ::"i"(IA32_MSR_APIC_BASE), "i"(IA32_APIC_ENABLE)
        : "eax", "ecx", "edx"
    );

    // initialize LAPIC
    uint32_t apic_id = _apic_read_register(APIC_IDR) >> 24;
    uint32_t apic_version = _apic_read_register(APIC_VER);

    kernel_log(INFO, "ID: %h, Version: %h, Max LVT: %u",
                    apic_id,
                    apic_version & 0xff,
                    (apic_version >> 16) & 0xff);

    // initialize LVT
    _apic_setup_lvts();
    // set priority of interrupts
    // vector number >= 2*16+0
    _apic_write_register(APIC_TPR, TPR_DEFAULT(2, 0));

    // enable APIC
    uint32_t spiv = _apic_read_register(APIC_SPIVR);
    _apic_write_register(APIC_SPIVR, SVR_DEFAULT((spiv&~0xff)));

    return 1;
}

