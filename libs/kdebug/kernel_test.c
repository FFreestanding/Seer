#include <kdebug/kernel_test.h>
#include <stdint.h>
#include <kernel_io/memory.h>
// #include <timer/clock.h>
#include <apic/rtc.h>
#include <apic/timer.h>
// #include <apic/keyboard.h>

void 
test_interrupt3(){
    asm("int $1");
}

void 
test_interrupt2(){
    test_interrupt3();
}

void 
test_interrupt1(){
    test_interrupt2();
}

void 
test_interrupt(){
    test_interrupt1();
}

void
test_heap_management(){
    // test kmalloc & kfree
    uint8_t** arr = (uint8_t**) kmalloc(10*sizeof(uint8_t*));
    for (uint32_t i = 0; i < 10; i++)
    {
        arr[i] = (uint8_t*) kmalloc((i+1)*2);
    }
    for (uint32_t i = 0; i < 10; i++)
    {
        if (!kfree(arr[i]))
        {
            kernel_log(WARN, "FREE ERROR %i", i);
        }
    }
    
    uint8_t* big_ = kmalloc(8192);
    big_[0]=123;
    big_[1]=23;
    big_[2]=1;
    kernel_log(INFO, "%u, %u, %u", big_[0], big_[1], big_[2]);
    if (!kfree(arr) || !kfree(big_))
    {
        kernel_log(WARN, "FREE ERROR arr big_");
    }
    
    uint8_t* bad1 = kmalloc(123);
    uint8_t* bad2 = kmalloc(1);
    *((uint32_t*)(bad1-4))=0xc2343312UL;
    if (!kfree(bad1) )
    {
        kernel_log(WARN, "FREE ERROR bad1");
    }
    if (!kfree(bad2-2))
    {
        kernel_log(WARN, "FREE ERROR bad2");
    }
    if (!kfree(bad2))
    {
        kernel_log(WARN, "FREE ERROR bad2 2");
    }
}

void 
start_clocks(void* p)
{
    VGA_Manager* v = get_vga_manager_instance();
    uint8_t x = v->vga_x;
    uint8_t y = v->vga_y;
    v->vga_x = VGA_WIDTH;
    v->vga_y = VGA_HEIGTH-2;
    if (!(rtc_read_reg(rtc_register_b)&0x4))
    {
        kprintf("%u/%u/%u %u:%u:%u   ",
            bcd2dec(rtc_read_reg(rtc_year))+2000,
            bcd2dec(rtc_read_reg(rtc_month)),
            bcd2dec(rtc_read_reg(rtc_day_of_month)),
            (bcd2dec(rtc_read_reg(rtc_hours))+8)%24,
            bcd2dec(rtc_read_reg(rtc_minutes)),
            bcd2dec(rtc_read_reg(rtc_seconds))
        );
    }

    v->vga_x = x;
    v->vga_y = y;

}

void test_clocks()
{
    timer_run_second(1, start_clocks, 0, 1);
}
