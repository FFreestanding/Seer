#ifndef _RTC_H_
#define _RTC_H_
#include <stdint.h>

/* RTC Indexed Registers */
// https://edc.intel.com/content/www/cn/zh/design/products-and-solutions/processors-and-chipsets/comet-lake-u/intel-400-series-chipset-on-package-platform-controller-hub-register-database/rtc-indexed-registers/

#define RTC_INDEX_PORT 0x70
#define RTC_TARGET_PORT 0x71
#define rtc_seconds 0x00
#define rtc_seconds_alarm 0x01
#define rtc_minutes 0x02
#define rtc_minutes_alarm 0x03
#define rtc_hours 0x04
#define rtc_hours_alarm 0x05
#define rtc_day_of_week 0x06
#define rtc_day_of_month 0x07
#define rtc_month 0x08
#define rtc_year 0x09
#define rtc_register_a 0x0a
#define rtc_register_b 0x0b
#define rtc_register_c 0x0c
#define rtc_register_d 0x0d

// https://web.stanford.edu/class/cs140/projects/pintos/specs/mc146818a.pdf
// TABLE 4 - DIVIDER CONFIGURATIONS

// 33kHz
#define RTC_DEFAULT_DIVIDER (0b010<<4)

// TABLE 5 - PERIODIC INTERRUPT RATE AND SQUARE WAVE OUTPUT FREQUENCY

// 1024 Hz
#define RTC_INTERRUPT_DEFAULT_FREQUENCY 0b0110

// When UIP is a "1", the update cycle is in progress or will soon begin. 
#define SET_UIP (1<<7)

// REGISTER B ($0B) bit6 PIE
/* A program writes a "1" to the PIE bit in order to receive periodic
interrupts at the rate specified by the RS3, RS2, RS1, RS0 bits in Register A */
#define RTC_TIMER_ON (1<<6)

#define REDIRECT_RTC_TIMER_VECTOR 210


// Ref: IBM PC AT Technical Reference Mar84
// IRQ 8 - Realtime Clock Interrupt
#define RTC_IRQ 8

void 
_rtc_init();

uint8_t
rtc_read_reg(uint8_t reg);

void
rtc_write_reg(uint8_t reg, uint8_t value);

void
rtc_enable_timer();

void
rtc_disable_timer();


#endif // _RTC_H_