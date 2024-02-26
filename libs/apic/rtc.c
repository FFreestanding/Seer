#include <apic/rtc.h>
#include <apic/cpu.h>
#include <kernel_io/memory.h>

void 
_rtc_init()
{
    uint8_t regA = rtc_read_reg(rtc_register_a|0x80);
    // regA |= RTC_INTERRUPT_DEFAULT_FREQUENCY | RTC_DEFAULT_DIVIDER;
    regA = (regA&(~0x7f)) | RTC_INTERRUPT_DEFAULT_FREQUENCY | RTC_DEFAULT_DIVIDER;
    rtc_write_reg(rtc_register_a|0x80, regA);
    rtc_disable_timer();
}

uint8_t
rtc_read_reg(uint8_t reg_selector)
{
    io_outb(RTC_INDEX_PORT, reg_selector);
    return io_inb(RTC_TARGET_PORT);
}

void
rtc_write_reg(uint8_t reg, uint8_t value)
{
    io_outb(RTC_INDEX_PORT, reg);
    io_outb(RTC_TARGET_PORT, value);
}


void
rtc_enable_timer() {
    uint8_t regB = rtc_read_reg(rtc_register_b | 0x80);
    rtc_write_reg(rtc_register_b | 0x80, regB | RTC_TIMER_ON);
}

void
rtc_disable_timer() {
    uint8_t regB = rtc_read_reg(rtc_register_b|0x80);
    rtc_write_reg(rtc_register_b|0x80, regB & ~RTC_TIMER_ON);
}