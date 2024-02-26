#include <apic/keyboard.h>
#include <apic/acpi.h>
#include <kernel_io/memory.h>
#include <apic/cpu.h>
#include <idt/interrupts.h>
#include <apic/timer.h>
#include <apic/ioapic.h>
/*
esc
0x76 0xf0 0x76
F1
0x5 0xf0 0x5
F2
0x6 0xf0 0x6
F3
0x4 0xf0 0x4
F4
0xc 0xf0 0xc
F5
0x3 0xf0 0x3
F6
0xb 0xf0 0xb
F7
0x83 0xf0 0x83
F8
0xa 0xf0 0xa
F9
0x1 0xf0 0x1
F10
0x9 0xf0 0x9
F11
0x78 0xf0 0x78
F12
0x7 0xf0 0x7
DELETE
0xe0 0x71 0xe0 0xf0 0x71
`
0xe 0xf0 0xe
1
0x16 0xf0 0x16
2
0x1e
3
0x26
4
0x25
5
0x2e
6
0x36
7
0x3d 0xf0 0x3d
8
0x3e
9
0x46
0
0x45
-
0x4e
=
0x55
backspace
0x66
TAP
0xd 0xf0 0xd
q
0x15
w
0x1d
e
0x24
r
0x2d
t
0x2c
y
0x35
u
0x3c
i
0x43
o
0x44
p
0x4d
[
0x54
]
0x5b
\
0x5d
CAPSLOCK
0x58 0xf0 0x58
a
0x1c
s
0x1b
d
0x23
f
0x2b
g
0x34
h
0x33
j
0x3b
k
0x42
l
0x4b
;
0x4c
'
0x52
ENTER
0x5a
LEFT SHIFT
0x12 0xf0 0x12
z
0x1a
x
0x22
c
0x21
v
0x2a
b
0x32
n
0x31
m
0x3a
,
0x41
.
0x49
/
0x4a
RIGHT SHIFT
0x59 0xf0 0x59
LEFT CTRL
0x14
LEFT FN

WIN

LEFT ALT
0x11
SPACE
0x29
RIGHT ALT
0x11 0xe0 0xf0 0x11
PERTSC
0xe0 0x12 0xe0 0x7c 0xe0 0xf0 0x7c 0xe0 0xf0 0x12
RIGHT CTRL
0xe0 0x14 0xe0 0xf0 0x14
LEFT
0xe0 0x6b 0xe0 0xf0 0x6b
RIGHT
0xe0 0x74 0xe0 0xf0 0x74
UP
0xe0 0x75 0xe0 0xf0 0x75
DOWN
0xe0 0x72 0xe0 0xf0 0x72
*/


static uint8_t scancode_set2[] = {
    0,0,0,0, 0,0,0,0, 0,0,0,0, 0,'  ','`',0,//0x0-0xf
    0,0,0,0, 0,'q','1',0, 0,0,'z','s', 'a','w','2',0, //0x10-0x1f
    0,'c','x','d', 'e','4','3',0, 0,0,'v','f', 't','r','5',0, //0x20-0x2f
    0,'n','b','h', 'g','y','6',0, 0,0,'m','j', 'u','7','8',0, //0x30-0x3f
    0,',','k','i', 'o','0','9',0, 0,'.','/','l', ';','p','-',0, //0x40-0x4f
    0,0,'\'',0, '[','=',0,0, 0,0,'\n',']', 0,'\\',0,0,//0x50-0x5f
    0,0,0,0, 0,0,' ',0, 0,0,0,0, 0,0,0,0//0x60-0x6f
};

static uint8_t scancode_capslock_or_shift[] = {
    0,0,0,0, 0,0,0,0, 0,0,0,0, 0,'  ','~',0,//0x0-0xf
    0,0,0,0, 0,'Q','!',0, 0,0,'Z','S', 'A','W','@',0, //0x10-0x1f
    0,'C','X','D', 'E','$','#',0, 0,0,'V','F', 'T','R','%',0, //0x20-0x2f
    0,'N','B','H', 'G','Y','^',0, 0,0,'M','J', 'U','&','*',0, //0x30-0x3f
    0,'<','K','I', 'O',')','(',0, 0,'>','?','L', ':','P','_',0, //0x40-0x4f
    0,0,'\"',0, '{','+',0,0, 0,0,'\n','}', 0,'|',0,0,//0x50-0x5f
    0,0,0,0, 0,0,' ',0, 0,0,0,0, 0,0,0,0//0x60-0x6f
};

static uint8_t number_to_char[] = {
    ')','!','@','#','$','%','^','&','*','('
};

scan_code_buffer_manager*  
scan_code_buffer_manager_get()
{
    static scan_code_buffer_manager manager;
    return &manager;
}

void 
scan_code_buffer_manager_init(scan_code_buffer_manager* manager)
{
    manager->buffer_head=0;
    manager->buffer_tail=1;
    manager->locked=0;
}

static void 
ps2_keyboard_routine(isr_param* param)
{
    static uint8_t mode=0;
    static uint8_t shift_down=0;
    static uint8_t capslock_down=0;
    static uint8_t ret=0;

    // kprintf("\n%s ret=%h\n", "ps2_keyboard_routine", ret);
    if (ret)
    {
        if (ret==io_inb(PS2_DEVICE_REGISTER) || ret==MODE_CLOSE)
        {
            ret=0;
            return;
        }
    }
    
    mode = io_inb(PS2_DEVICE_REGISTER);
    io_delay(300);

    // kprintf(" scan code %h\n ", mode);
    uint8_t code;
    if (mode==MODE_0XE0_OPEN)
    {//match code in left shift, right shift, up, down, left, right

        // wait_output_full();
        switch (code=io_inb(PS2_DEVICE_REGISTER))
        {
        case KEY_UP:
            // kprintf("up code %h\n ", code);
            vga_move_cursor_up();
            break;
        case KEY_DOWN:
            // kprintf("down code %h\n ", code);
            vga_move_cursor_down();
            break;
        case KEY_LEFT:
            // kprintf("left code %h\n ", code);
            vga_move_cursor_left();
            break;
        case KEY_RIGHT:
            // kprintf("right code %h\n ", code);
            vga_move_cursor_right();
            break;
        case MODE_CLOSE:
            // kprintf("close code %h\n ", code);
            // wait_output_full();
            // io_inb(PS2_DEVICE_REGISTER);
            // io_delay(300);
            break;
        case KEY_DELETE:
            vga_delete();
            break;
        default:
            break;
        }
        ret=code; 
    }
    else if (mode==MODE_CLOSE)
    {
        // kprintf("%s", "MODE_CLOSE");
        // wait_output_full();
        // uint8_t a = io_inb(PS2_DEVICE_REGISTER);
        // kprintf(" 1:%h ", a);
        io_delay(300);
        shift_down=0;
        ret=mode;
    }
    else
    {
        switch (mode)
        {
        case KEY_CAPSLOCK:
            capslock_down^=1;
            // kprintf(" capslock_down:%h ", capslock_down);
            break;
        case KEY_LEFT_SHIFT:
            shift_down^=1;
            break;
        case KEY_RIGHT_SHIFT:
            shift_down^=1;
            break;
        case KEY_BACKSPACE:
            vga_move_cursor_left();
            vga_delete();
            break;
        default:
            if (capslock_down && scancode_set2[mode] >= 'a' && scancode_set2[mode] <= 'z' || shift_down)
            {
                kprintf("%c", scancode_capslock_or_shift[mode]);
            }
            else
            {
                kprintf("%c", scancode_set2[mode]);
            }
            break;
        }
    }
    
}

// https://wiki.osdev.org/%228042%22_PS/2_Controller
void 
_ps2_controller_init()
{
    // Determine if the PS/2 Controller Exists
    acpi_context* ac = get_acpi_context_instance();
    if (ac->fadt.creator_revision > 1)
    {
        _assert(ac->fadt.iapc_boot_arch & IAPC_ARCH_8042, 
                "No 8042 detected.");
        kernel_log(WARN, "APIC version > v1");
    }
    else
    {
        kernel_log(WARN, "Outdated APIC version: v1");
    }
    disable_interrupts();
    // Disable Devices
    ps2_send_cmd_only(PS2_CONTROLLER_REGISTER, 0xad);
    ps2_send_cmd_only(PS2_CONTROLLER_REGISTER, 0xa7);
    // Flush The Output Buffer
    io_inb(PS2_DEVICE_REGISTER);
    io_delay(500);
    // Set the Controller Configuration Byte
    uint8_t result = ps2_send_controller_cmd(0x20);
    result = result &  ~(0b1 | 0b1<<1 | 0b1<<6);
    ps2_send_controller_cmd(0x60);
    io_outb(PS2_DEVICE_REGISTER, result&0xff);
    // Perform Controller Self Test
    result = ps2_send_controller_cmd(0xaa);
    _assert(result==0x55, "Perform Controller Self Test ERROR");
    // Perform Interface Tests
    kernel_log(INFO, "%h", ps2_send_controller_cmd(0xab));
    _assert(ps2_send_controller_cmd(0xab)==0, "Perform Interface Tests ERROR");

    // Enable Keyboard
    ps2_send_cmd_only(PS2_CONTROLLER_REGISTER, 0xae);
    result = ps2_send_controller_cmd(0x20);
    result |= 0x1;
    ps2_send_cmd_only(PS2_CONTROLLER_REGISTER, 0x60);
    ps2_send_cmd_only(PS2_DEVICE_REGISTER, result);

    scan_code_buffer_manager_init(scan_code_buffer_manager_get());
    interrupt_routine_subscribe(PS2_KBD_REDIRECT_VECTOR, ps2_keyboard_routine);

    ioapic_redirect(
        ioapic_get_irq(get_acpi_context_instance(), PS2_KBD_IRQ),
        PS2_KBD_REDIRECT_VECTOR,
        0,
        IOAPIC_DELIVERY_MODE(IOAPIC_DELIVERY_MODE_FIXED)
    );
    
    enable_interrupts();

    // enable cursor light
    io_outb(0x3d4, 0x0a);
    io_outb(0x3d5, (io_inb(0x3d5)&0xc0) | 13);
    io_outb(0x3d4, 0x0b);
    io_outb(0x3d5, (io_inb(0x3d5)&0xe0) | 15);
}

