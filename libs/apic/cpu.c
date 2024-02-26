#include <apic/cpu.h>
#include <cpuid.h>

uint32_t _cpu_has_apic()
{
    // reference: Intel manual, Volume 3, section 10.4.2
    reg32 eax = 0, ebx = 0, edx = 0, ecx = 0;
    if(__get_cpuid(1, &eax, &ebx, &ecx, &edx))
    {
        return (edx & 0x100);
    }
    else
    {
        return 0;
    }
}
