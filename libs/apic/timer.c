#include <apic/timer.h>
#include <apic/cpu.h>
#include <kernel_io/heap.h>
#include <kernel_io/memory.h>
#include <idt/interrupts.h>


void
_timer_init()
{
    timer_task_manager* ttm = get_timer_task_manager_instance();
    timer_task_manager_init(ttm);
    
    disable_interrupts();

    _apic_write_register(APIC_TIMER_LVT, LVT_TIMER_DEFAULT(REDIRECT_APIC_TIMER_VECTOR));
    _apic_write_register(APIC_TIMER_DCR, APIC_TIMER_DIV128);
    
    interrupt_routine_subscribe(REDIRECT_APIC_TIMER_VECTOR, temp_apic_timer_routine);
    interrupt_routine_subscribe(REDIRECT_RTC_TIMER_VECTOR, temp_rtc_timer_routine);
    
    rtc_enable_timer();
    
    _apic_write_register(APIC_TIMER_ICR, CONSTANT_ICR);

    enable_interrupts();

    while (!ttm->frequency_measurement_over);
    kernel_log(INFO, "frequency measurement over");
    
    uint32_t interrupt_counters_per_second = 1024*2;
    ttm->setting_running_frequence=interrupt_counters_per_second;
    
    interrupt_routine_unsubscribe(REDIRECT_APIC_TIMER_VECTOR);
    interrupt_routine_unsubscribe(REDIRECT_RTC_TIMER_VECTOR);
    
    _apic_write_register(APIC_TIMER_LVT, 
        LVT_TIMER_MODE(0b01) | (REDIRECT_APIC_TIMER_VECTOR) | LVT_DELIVERY_MODE(0b000));
    
    interrupt_routine_subscribe(REDIRECT_APIC_TIMER_VECTOR, timer_task_list_update);
    _apic_write_register(APIC_TIMER_ICR, 
                    ttm->apic_frequence/interrupt_counters_per_second);
    kernel_log(INFO, "frequency_measurement_over %u", ttm->frequency_measurement_over);
    kernel_log(INFO, "apic_frequence %u", ttm->apic_frequence);
    kernel_log(INFO, "per second conters: %u", ttm->apic_frequence/interrupt_counters_per_second);
        
}

timer_task_manager* 
get_timer_task_manager_instance()
{
    static timer_task_manager ttm;
    return &ttm;
}

void
timer_task_manager_init(timer_task_manager* ttm)
{
    _assert(
        ttm->task_head = (timer_task*) kmalloc(sizeof(timer_task)),
        "task_head init error"
    );
    llist_init_head(&ttm->task_head->link);
}

uint8_t timer_run
(uint32_t ticks, void (*task_caller)(void*), void* parameter_ptr, uint8_t task_is_periodic)
{
    timer_task* timer = (timer_task*)kmalloc(sizeof(timer_task));
    kernel_log(INFO, "timer run");
    if (!timer) {return 0;};

    timer->counter = timer->deadline = ticks;
    timer->task_caller = task_caller;
    timer->task_is_periodic = task_is_periodic;
    timer->parameter_ptr = parameter_ptr;

    llist_append(get_timer_task_manager_instance()->task_head, &timer->link);

    return 1;
}

void timer_run_second
(uint32_t second, void (*task_caller)(void*), void* parameter_ptr, uint8_t task_is_periodic)
{
    _assert(
        timer_run(get_timer_task_manager_instance()
                    ->setting_running_frequence*second,
                    task_caller, parameter_ptr, 1),
        "timer run error"
    )
}

void timer_run_ms
(uint32_t millisecond, void (*task_caller)(void*), void* parameter_ptr, uint8_t task_is_periodic)
{
    _assert(
        timer_run(get_timer_task_manager_instance()
                    ->setting_running_frequence*millisecond/1000,
                    task_caller, parameter_ptr, 1),
        "timer run error"
    )
}

uint8_t
bcd2dec(uint8_t bcd)
{
    return ((bcd & 0xF0) >> 1) + ((bcd & 0xF0) >> 3) + (bcd & 0xf);
}