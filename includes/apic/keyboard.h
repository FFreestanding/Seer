#ifndef _KEYBOARD_H_
#define _KEYBOARD_H_
#include <stdint.h>
#include <idt/interrupts.h>
#include <kernel_io/memory.h>
#include <stdatomic.h>
/* Reference IBM Personal System 2 Model 80 Technical Reference */

// Controller Status register
#define PS2_CONTROLLER_REGISTER 0x64
#define PS2_DEVICE_REGISTER 0x60


// Parity Error
// General Time-Out
// Auxiliary Device Output Buffer Full
// Inhibit Switch
// Command/Data
// System Flag
// Input Buffer Full
#define PS2_INPUT_BUFFER_FULL 0b10
// Output Buffer Full 
#define PS2_OUTPUT_BUFFER_FULL 0b1

#define IAPC_ARCH_8042     0x2

#define PS2_KBD_REDIRECT_VECTOR 201

#define PS2_KBD_IRQ 1

#define KEY_CAPSLOCK 0x58
// LEFT SHIFT
// 0x12 0xf0 0x12
// RIGHT SHIFT
// 0x59 0xf0 0x59

#define KEY_LEFT 0x6b
#define KEY_RIGHT 0x74
#define KEY_UP 0x75
#define KEY_DOWN 0x72
#define KEY_LEFT_SHIFT 0x12
#define KEY_RIGHT_SHIFT 0x59
#define KEY_BACKSPACE 0x66
#define KEY_DELETE 0x71

#define MODE_0XE0_OPEN 0xe0

#define MODE_CLOSE 0xf0

#define wait_output_full() while(!io_inb(PS2_CONTROLLER_REGISTER)&PS2_OUTPUT_BUFFER_FULL);
#define wait_input_empty() while(io_inb(PS2_CONTROLLER_REGISTER)&PS2_INPUT_BUFFER_FULL)

#define lock_keyboard_buffer(manager) atomic_store(&manager->locked, 1)
#define unlock_keyboard_buffer(manager) atomic_store(&manager->locked, 0)

void 
_ps2_controller_init();

static uint8_t 
ps2_send_controller_cmd(uint8_t cmd);

static uint8_t 
ps2_send_device_cmd(uint8_t cmd);

static uint8_t 
ps2_send_cmd_only(uint8_t reg, uint8_t cmd);

static void 
ps2_keyboard_routine(isr_param* param);

static uint8_t 
ps2_send_controller_cmd(uint8_t cmd)
{
    wait_input_empty();
    io_outb(PS2_CONTROLLER_REGISTER, cmd);

    wait_output_full();
    io_inb(PS2_DEVICE_REGISTER);
}

static uint8_t 
ps2_send_device_cmd(uint8_t cmd)
{
    wait_input_empty();
    io_outb(PS2_DEVICE_REGISTER, cmd);

    wait_output_full();
    return io_inb(PS2_DEVICE_REGISTER);
}

static uint8_t 
ps2_send_cmd_only(uint8_t reg, uint8_t cmd)
{
    wait_input_empty();
    io_outb(reg, cmd);
}

typedef struct 
{
    uint8_t scan_code_buffer[10];
    uint8_t buffer_head;
    uint8_t buffer_tail;
    _Atomic uint8_t locked;
} scan_code_buffer_manager;

scan_code_buffer_manager*  
scan_code_buffer_manager_get();

void 
scan_code_buffer_manager_init(scan_code_buffer_manager* manager);

#endif // _KEYBOARD_H_