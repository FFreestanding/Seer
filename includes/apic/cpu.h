#ifndef _CPU_H_
#define _CPU_H_
#include <stdint.h>

#define disable_interrupts() asm volatile("cli")
#define enable_interrupts() asm volatile("sti")

typedef uint32_t reg32;

#pragma pack(1)
typedef struct
{
    reg32 eax;
    reg32 ebx;
    reg32 ecx;
    reg32 edx;
    reg32 edi;
    reg32 ebp;
    reg32 esi;
    reg32 esp;
} gp_regs;
#pragma pack()

uint32_t _cpu_has_apic();

static inline void
io_delay(int counter);

static inline void
io_delay(int counter)
{
    asm volatile (
        "   test %0, %0\n"
        "   jz 1f\n"
        "2: dec %0\n"
        "   jnz 2b\n"
        "1: dec %0"::"a"(counter));
}

static inline void
cpu_invplg(void* va)
{
    asm volatile("invlpg (%0)" ::"r"((uintptr_t)va) : "memory");
}

static inline void
io_outb(int port, uint8_t data)
{
    asm volatile("outb %0, %w1" : : "a"(data), "d"(port));
}

static inline uint8_t
io_inb(int port)
{
    uint8_t data;
    asm volatile("inb %w1,%0" : "=a"(data) : "d"(port));
    return data;
}

static inline void
io_outl(int port, uint32_t data)
{
    asm volatile("outl %0, %w1" : : "a"(data), "d"(port));
}

static inline uint32_t
io_inl(int port)
{
    uint32_t data;
    asm volatile("inl %w1,%0" : "=a"(data) : "d"(port));
    return data;
}

#endif // _CPU_H_