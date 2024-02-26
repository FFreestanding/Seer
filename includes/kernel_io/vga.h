#ifndef __VGA_H
#define __VGA_H 1

#include <stdint.h>

#define VGA_WIDTH 80
#define VGA_HEIGTH 25

#define VGA_START_POINTER 0xB8000
#define VGA_VIRTUAL_ADDRESS 0xB0000000
#define VGA_BUFFER_SIZE 4096

#define VGA_COLOR_BLACK 0
#define VGA_COLOR_BLUE 1
#define VGA_COLOR_GREEN 2
#define VGA_COLOR_CYAN 3
#define VGA_COLOR_RED 4
#define VGA_COLOR_MAGENTA 5
#define VGA_COLOR_BROWN 6
#define VGA_COLOR_LIGHT_GREY 7
#define VGA_COLOR_DARK_GREY 8
#define VGA_COLOR_LIGHT_BLUE 9
#define VGA_COLOR_LIGHT_GREEN 10
#define VGA_COLOR_LIGHT_CYAN 11
#define VGA_COLOR_LIGHT_RED 12
#define VGA_COLOR_LIGHT_MAGENTA 13
#define VGA_COLOR_LIGHT_BROWN 14
#define VGA_COLOR_WHITE 15

#define INFO 0
#define WARN 1
#define ERROR 2 

typedef unsigned short VGA_COLOR;

typedef struct VGA_Manager
{
    unsigned short* vga_ptr;
    unsigned int vga_theme_color;
    unsigned int vga_x;
    unsigned int vga_y;
} VGA_Manager;

void 
vga_manager_init(VGA_Manager* vga_manager);

VGA_Manager*
get_vga_manager_instance();

void
vga_putchar(VGA_Manager*, char);

void
vga_putstr(VGA_Manager*, char*);

void
vga_scroll_up();

void
vga_clear(VGA_Manager*);

void
vga_set_theme(VGA_Manager*, VGA_COLOR, VGA_COLOR);

void 
tty_sync_cursor();

void 
tty_set_cursor(uint8_t x, uint8_t y);

void
vga_putchar_x_y_temporarily(VGA_Manager*, char, uint8_t x, uint8_t y);

void
vga_putstr_x_y_temporarily(VGA_Manager*, char*, uint8_t x, uint8_t y);

void 
vga_backspace();

void 
vga_delete();

void 
vga_move_cursor_up();

void 
vga_move_cursor_left();

void 
vga_move_cursor_right();

void 
vga_move_cursor_down();

#endif