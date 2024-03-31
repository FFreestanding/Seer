#ifndef __KDEBUG_H
#define __KDEBUG_H 1

#include <stdint.h>
#include <boot/multiboot.h>
/*
|entry_num uint32|
entry_size=
   uint16_t     +char*len+1  +(uint16+uint32    +uint16     +char*len+1 )
|filename_length|"kernel.c"\0|21     |0xc010c2e6|name_length|func_name\0|22|0xc010c2ec|1|\0|
(filename)


1.get eip / (uint32_t) eip_addr
2.get an entry address / uint8_t* entry_ptr
3.pass filename / entry_ptr = entry_ptr + 16+ *(uint16_t*)entry_ptr
4.pass line number / entry_ptr += 16 / addr = entry_ptr
5.if addr == eip
    YES: get filename, line number, function name and print them
        get new eip
    NO: pass function name
        pass line number
6.if no found / return
*/

// pm_mgr need to point at the first byte of the entry
#define GET_FILENAME_LEN(p) (*(uint16_t*)(p))

// pm_mgr need to point at the first byte of the entry
#define POINT_AT_FILENAME(p) ((uint16_t*)(p) + 1)

// pm_mgr need to point at the first byte of the entry
#define GET_LINE_INFO_LEN_ADDRESS(p) ((uint32_t*)((uint8_t*)POINT_AT_FILENAME(p)+GET_FILENAME_LEN(p)))

// pm_mgr need to point at the first byte of the entry
#define GET_LINE_INFO_LEN(p) (*GET_LINE_INFO_LEN_ADDRESS(p))


// pm_mgr need to point at the first byte of the entry
#define GET_FIRST_LINE_INFO_ADDRESS(p) ((uint32_t*)GET_LINE_INFO_LEN_ADDRESS(p)+1)


// pm_mgr need to point at the first byte of the line_info
#define GET_LINE_INFO_FUNCNAME_LEN(p) (*(uint16_t*)((uint8_t*)(p) + 16/8 + 32/8))

// pm_mgr need to point at the first byte of the line_info
#define GET_LINE_INFO_FUNCNAME_ADDRESS(p) ((uint8_t*)(p) + 16/8 + 32/8 + 16/8)

// pm_mgr need to point at the first byte of the line_info
#define GET_NEXT_LINE_INFO_ADDRESS(p) ((uint32_t)(GET_LINE_INFO_FUNCNAME_ADDRESS(p)+GET_LINE_INFO_FUNCNAME_LEN(p)))

// pm_mgr need to point at the first byte of the entry
#define GET_NEXT_ENTRY_ADDRESS(line_info_ptr) ((uint32_t*)GET_NEXT_LINE_INFO_ADDRESS(line_info_ptr))

// pm_mgr need to point at the first byte of the line_info
#define GET_INSTRUCTION_ADDRESS(p) (*(uint32_t*)((uint16_t*)p+1))

// pm_mgr need to point at the first byte of the line_info
#define GET_LINE_INFO_LINE_NUMBER(p) (*(uint16_t*)p)

void
save_debug_info(uint32_t mod_address);

void 
printstack(uint32_t esp, uint32_t eip);

uint8_t 
find_debug_info(uint32_t eip);

#endif