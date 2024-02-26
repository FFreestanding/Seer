#ifndef _TIMER_H_
#define _TIMER_H_
#include <stdint.h>
#include <data_structure/llist.h>
#include <apic/rtc.h>
#include <apic/apic.h>
#include <kernel_io/heap.h>
#include <kernel_io/memory.h>
#include <idt/interrupts.h>

typedef struct 
{
    struct llist_header link;
    uint32_t deadline;
    uint32_t counter;
    void* parameter_ptr;
    uint8_t task_is_periodic;
    void (*task_caller)(void*);
} timer_task;


typedef struct 
{
    timer_task* task_head;
    uint32_t apic_frequence;
    uint32_t setting_running_frequence;
    uint8_t frequency_measurement_over;
} timer_task_manager;

void
_timer_init();

timer_task_manager* 
get_timer_task_manager_instance();

void
timer_task_manager_init(timer_task_manager* ttm);

static void
timer_task_list_update(isr_param* param);

uint8_t timer_run
(uint32_t ticks, void (*task_caller)(void*), void* parameter_ptr, uint8_t task_is_periodic);

void timer_run_second
(uint32_t second, void (*task_caller)(void*), void* parameter_ptr, uint8_t task_is_periodic);

void timer_run_ms
(uint32_t second, void (*task_caller)(void*), void* parameter_ptr, uint8_t task_is_periodic);

uint8_t
bcd2dec(uint8_t bcd);

static void 
temp_apic_timer_routine(isr_param* p);

static void 
temp_rtc_timer_routine(isr_param* p);

static void 
temp_rtc_timer_routine(isr_param* p)
{
    // kernel_log(INFO, "rtc timer click");
    timer_task_manager* ttm = get_timer_task_manager_instance();
    ttm->apic_frequence++;
    // kernel_log(INFO, "apic_frequence %u", ttm->apic_frequence);
    rtc_read_reg(rtc_register_c);
}

static void 
temp_apic_timer_routine(isr_param* p)
{
    // kernel_log(INFO, "apic timer click");
    timer_task_manager* ttm = get_timer_task_manager_instance();
    kernel_log(WARN, "counter %h", ttm->apic_frequence);
    ttm->apic_frequence = CONSTANT_ICR / ttm->apic_frequence * 1024;//RTC_INTERRUPT_DEFAULT_FREQUENCY
    ttm->frequency_measurement_over = 1;
    kernel_log(WARN, "apic_frequence %h", ttm->apic_frequence);
    
    rtc_disable_timer();

}

static void
timer_task_list_update(isr_param* param)
{
    timer_task* pos, *next;
    timer_task* head = get_timer_task_manager_instance()->task_head;

    _assert(head, "timer_task head null");
    llist_for_each(pos, next, &head->link, link)
    {
        if (--pos->counter)
        {
            continue;
        }
        if (pos->task_caller)
        {
            pos->task_caller(pos->parameter_ptr);
        }
        
        if (pos->task_is_periodic)
        {
            pos->counter = pos->deadline;
        }
        else
        {
            llist_delete(&pos->link);
            _assert(kfree(pos),"task list update error");
        }
    }
}

#endif // _TIMER_H_