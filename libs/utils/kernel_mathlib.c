#include <utils/kernel_mathlib.h>

uint32_t
kernel_pow(uint32_t base, uint32_t exponent)
{
    if (exponent==0)
    {
        return 1;
    }
    
    for (uint32_t i = 1; i < exponent; i++)
    {
        base *= base;
    }

    return base;
}