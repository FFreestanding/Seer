#include <idt/interrupts.h>
#include <idt/idt.h>
#include <kernel_io/vga.h>
#include <kernel_io/memory.h>
#include <kdebug/kdebug.h>
#include <apic/apic.h>
#include <apic/keyboard.h>

void 
interrupt_handler(isr_param* param) {
    static int i = 1;
    if (param->vector<=20)
    {
        if (i==2) { while (1); }
        kernel_log(WARN, "param->vector: %u, param->eip: %h, error code: %h", param->vector, param->eip, param->err_code);
        ++i;
        printstack(param->registers.ebp, param->eip);
        --i;
        
    }

    switch (param->vector)
    {
        case INTERRUPT_DIVISION_EXCEPTION:
            isr0(param);
            return;
        case INTERRUPT_DEBUG_EXCEPTION:
            isr1(param);
            return;
        case INTERRUPT_NMI:
            isr2(param);
            return;
        case INTERRUPT_BREAKPOINT:
            isr3(param);
            return;
        case INTERRUPT_OVERFLOW:
            isr4(param);
            return;
        case INTERRUPT_BOUND_RANGE_EXCEEDED:
            isr5(param);
            return;
        case INTERRUPT_INVALID_OPCODE:
            isr6(param);
            return;
        case INTERRUPT_DEVICE_NOT_AVAILABLE:
            isr7(param);
            return;
        case INTERRUPT_DOUBLE_FAULT:
            isr8(param);
            return;
        // case INTERRUPT_COPROCESSOR_SEGMENT_OVERRUN:
        //     isr9(param);
        //     break;
        case INTERRUPT_INVALID_TSS:
            isr10(param);
            return;
        case INTERRUPT_SEGMENT_NOT_PRESENT:
            isr11(param);
            return;
        case INTERRUPT_STACK_SEGMENT_FAULT:
            isr12(param);
            return;
        case INTERRUPT_GENERAL_PROTECTION:
            isr13(param);
            return;
        case INTERRUPT_PAGE_FAULT:
            isr14(param);
            return;
        // case INTERRUPT_RESERVED:
        //     isr15(param);
        //     return;
        case INTERRUPT_FLOATING_POINT_ERROR:
            isr16(param);
            return;
        case INTERRUPT_ALIGNMENT_CHECK:
            isr17(param);
            return;
        case INTERRUPT_MACHINE_CHECK:
            isr18(param);
            return;
        case INTERRUPT_SIMD_FLOATING_POINT_EXCEPTION:
            isr19(param);
            return;
        case INTERRUPT_VIRTUALIZATION_EXCEPTION:
            isr20(param);
            return;
        case INTERRUPT_CONTROL_PROTECTION_EXCEPTION:
            isr21(param);
            return;
    }
    void(*func_p)(isr_param*) = get_interrupt_routines_mananger_instance()->routines[param->vector];
    if (func_p)
    {
        func_p(param);
        if (param->vector!=REDIRECT_SPIV_VECTOR)
        {
            apic_done_servicing();
        }
        if (param->vector==PS2_KBD_REDIRECT_VECTOR)
        {
            // kernel_log(INFO, "PS2_KBD_REDIRECT_VECTOR");
            tty_sync_cursor();
        }
        
        return;
    }
    
    kernel_log(ERROR, "INT vector %u: err_code(%h) [cs:eip][%h:%h] Unknown",
            param->vector,
            param->err_code,
            param->cs,
            param->eip);

}

interrupt_routines_manager*
get_interrupt_routines_mananger_instance()
{
    static interrupt_routines_manager irm;
    return &irm;
}

void
_interrupt_routine_init()
{
    // interrupt_routine_subscribe();
}

void 
interrupt_routine_subscribe
(const uint8_t vector, void(*routine_ptr)(isr_param*))
{
    get_interrupt_routines_mananger_instance()->routines[vector]=routine_ptr;
}

void 
interrupt_routine_unsubscribe
(const uint8_t vector)
{
    get_interrupt_routines_mananger_instance()->routines[vector]=0;
}

void isr0 (isr_param* param) 
{
	kernel_log(ERROR, "!!! PANIC INTERRUPT_DIVISION_EXCEPTION 0 !!!");
}

void isr1 (isr_param* param) 
{
	kernel_log(ERROR, "!!! PANIC INTERRUPT_DEBUG_EXCEPTION 1 !!!");
}

void isr2 (isr_param* param) 
{
	kernel_log(ERROR, "!!! PANIC INTERRUPT_NMI 2 !!!");
}

void isr3 (isr_param* param) 
{
	kernel_log(ERROR, "!!! PANIC INTERRUPT_BREAKPOINT 3 !!!");
}

void isr4 (isr_param* param) 
{
	kernel_log(ERROR, "!!! PANIC INTERRUPT_OVERFLOW 4 !!!");
}

void isr5 (isr_param* param) 
{
	kernel_log(ERROR, "!!! PANIC INTERRUPT_BOUND_RANGE_EXCEEDED 5 !!!");
}

void isr6 (isr_param* param) 
{
	kernel_log(ERROR, "!!! PANIC INTERRUPT_INVALID_OPCODE 6 !!!");
}

void isr7 (isr_param* param) 
{
	kernel_log(ERROR, "!!! PANIC INTERRUPT_DEVICE_NOT_AVAILABLE 7 !!!");
}

void isr8 (isr_param* param) 
{
	kernel_log(ERROR, "!!! PANIC INTERRUPT_DOUBLE_FAULT 8 !!!");
}

void isr9 (isr_param* param) 
{
	kernel_log(ERROR, "!!! PANIC INTERRUPT_COPROCESSOR_SEGMENT_OVERRUN 9 !!!");
}

void isr10 (isr_param* param) 
{
	kernel_log(ERROR, "!!! PANIC INTERRUPT_INVALID_TSS 10 !!!");
}

void isr11 (isr_param* param) 
{
	kernel_log(ERROR, "!!! PANIC INTERRUPT_SEGMENT_NOT_PRESENT 11 !!!");
}

void isr12 (isr_param* param) 
{
	kernel_log(ERROR, "!!! PANIC INTERRUPT_STACK_SEGMENT_FAULT 12 !!!");
}

void isr13 (isr_param* param) 
{
	kernel_log(ERROR, "!!! PANIC INTERRUPT_GENERAL_PROTECTION 13 !!!");
}

void isr14 (isr_param* param) 
{
	kernel_log(ERROR, "!!! PANIC INTERRUPT_PAGE_FAULT 14 !!!");
}

void isr15 (isr_param* param) 
{
	kernel_log(ERROR, "!!! PANIC INTERRUPT_RESERVED 15 !!!");
}

void isr16 (isr_param* param) 
{
	kernel_log(ERROR, "!!! PANIC INTERRUPT_FLOATING_POINT_ERROR 16 !!!");
}

void isr17 (isr_param* param) 
{
	kernel_log(ERROR, "!!! PANIC INTERRUPT_ALIGNMENT_CHECK 17 !!!");
}

void isr18 (isr_param* param) 
{
	kernel_log(ERROR, "!!! PANIC INTERRUPT_MACHINE_CHECK 18 !!!");
}

void isr19 (isr_param* param) 
{
	kernel_log(ERROR, "!!! PANIC INTERRUPT_SIMD_FLOATING_POINT_EXCEPTION 19 !!!");
}

void isr20 (isr_param* param) 
{
	kernel_log(ERROR, "!!! PANIC INTERRUPT_VIRTUALIZATION_EXCEPTION 20 !!!");
}

void isr21 (isr_param* param) 
{
	kernel_log(ERROR, "!!! PANIC INTERRUPT_CONTROL_PROTECTION_EXCEPTION 21 !!!");
}

void isr253 (isr_param* param) 
{
	kernel_log(ERROR, "!!! PANIC 253 !!!");
}
