#ifndef __BOOT_TOOLS_H
#define __BOOT_TOOLS_H 1

#include <boot/multiboot.h>
#include <stdint.h>

void 
_save_multiboot_info(multiboot_info_t* boot_info, uint8_t* destination);

#endif