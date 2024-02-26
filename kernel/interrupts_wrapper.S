#define __ASM__ 1
#include <idt/idt.h>

.macro isr_template vector, no_error_code=1
    .global _asm_isr\vector/*引用_asm_isr的vector参数，会被vector替换*/
    .type _asm_isr\vector, @function/*看出函数使用*/
    _asm_isr\vector:
        .if \no_error_code
            pushl $0x0
        .endif
        pushl $\vector
        jmp interrupt_wrapper
.endm

.section .text
    isr_template INTERRUPT_DIVISION_EXCEPTION
    isr_template INTERRUPT_DEBUG_EXCEPTION
    isr_template INTERRUPT_NMI
    isr_template INTERRUPT_BREAKPOINT
    isr_template INTERRUPT_OVERFLOW
    isr_template INTERRUPT_BOUND_RANGE_EXCEEDED
    isr_template INTERRUPT_INVALID_OPCODE
    isr_template INTERRUPT_DEVICE_NOT_AVAILABLE
    isr_template INTERRUPT_DOUBLE_FAULT, no_error_code=0
    // isr_template INTERRUPT_COPROCESSOR_SEGMENT_OVERRUN
    isr_template INTERRUPT_INVALID_TSS, no_error_code=0
    isr_template INTERRUPT_SEGMENT_NOT_PRESENT, no_error_code=0
    isr_template INTERRUPT_STACK_SEGMENT_FAULT, no_error_code=0
    isr_template INTERRUPT_GENERAL_PROTECTION, no_error_code=0
    isr_template INTERRUPT_PAGE_FAULT, no_error_code=0
    // isr_template INTERRUPT_RESERVED
    isr_template INTERRUPT_FLOATING_POINT_ERROR
    isr_template INTERRUPT_ALIGNMENT_CHECK, no_error_code=0
    isr_template INTERRUPT_MACHINE_CHECK
    isr_template INTERRUPT_SIMD_FLOATING_POINT_EXCEPTION
    isr_template INTERRUPT_VIRTUALIZATION_EXCEPTION
    isr_template INTERRUPT_CONTROL_PROTECTION_EXCEPTION, no_error_code=0

    // isr_template APIC_ERROR_IV
    // isr_template APIC_LINT0_IV
    // isr_template APIC_TIMER_IV
    // isr_template APIC_SPIV_IV
    // isr_template RTC_TIMER_IV
    // isr_template PC_KBD_IV
    isr_template 250
    isr_template 251
    isr_template 252
    isr_template 253
    isr_template 210
    isr_template 201

    interrupt_wrapper:
        pushl %esp
        pushl %esi
        pushl %ebp
        pushl %edi
        pushl %edx
        pushl %ecx
        pushl %ebx
        pushl %eax

        movl %esp, %eax
        andl $0xfffffff0, %esp
        subl $16, %esp
        movl %eax, (%esp)

        call interrupt_handler
        popl %esp

        popl %eax
        popl %ebx
        popl %ecx
        popl %edx
        popl %edi
        popl %ebp
        popl %esi
        popl %esp
        
        addl $8, %esp
        
        iret
