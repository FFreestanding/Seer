#ifndef __MEMORY_H
#define __MEMORY_H 1
#include <stdint.h>
#include <kernel_io/vga.h>

// Warning: The length(byte size) parameter must be divided by 4
void
memory_copy_fast(uint32_t* source, uint32_t* destination, uint32_t length);

void
memory_copy(uint8_t* source, uint8_t* destination, uint32_t length);

// Warning: The length(byte size) parameter must be divided by 4
void
memory_set_fast(uint32_t* source, uint32_t value, uint32_t length);

void
memory_set(uint8_t* source, uint8_t value, uint32_t length);

void
kernel_log(const uint8_t log_type, const uint8_t* information, ...);

void log_int(VGA_Manager* log_vga, int n);

void log_uint(VGA_Manager* log_vga, uint32_t n);

void log_hex(VGA_Manager* log_vga, uint32_t n);

void
kprintf(const uint8_t* information, ...);

#endif