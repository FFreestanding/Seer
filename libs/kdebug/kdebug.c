#include <kdebug/kdebug.h>
#include <kernel_io/memory.h>
#include <paging/vmm.h>
#include <paging/page.h>
#include <kernel_io/valloc.h>
#include "common.h"

static uint32_t* debug_info_ptr;

void
save_debug_info(uint32_t mod_address)
{
    uint32_t *tmp = (uint32_t *) mod_address;
    mod_address = *tmp;
    ASSERT(mod_address == vmm_map_page(mod_address, mod_address),
           "save_debug_info");
    uint32_t info_bytes_number = *(uint32_t*)mod_address;
    uint32_t *src = vmm_map_pages(mod_address, mod_address,
                                  (info_bytes_number+0xfff)>>12);
    
    debug_info_ptr = (uint32_t*)vmm_alloc_pages((info_bytes_number+0xfff)>>12);
    memory_copy_fast(src+1, debug_info_ptr, info_bytes_number/4);
    for (uint32_t i = 0; i < ((info_bytes_number+0xfff)>>12); i++)
    {
        vmm_unmap_page((uint32_t)src + (i << 12));
    }
}

void 
printstack(uint32_t ebp, uint32_t eip)
{
    do
    {
        find_debug_info(eip);
        eip=*((uint32_t*)ebp+1);
    } while (ebp = *(uint32_t*)ebp);
}

uint8_t 
find_debug_info(uint32_t eip)
{
    uint32_t entry_num = *debug_info_ptr;
    // entry_ptr point at first entry
    uint32_t* entry_ptr = debug_info_ptr+1;
    uint32_t line_info_num = GET_LINE_INFO_LEN(entry_ptr);
    char* funcname_ptr = NULL_POINTER;
    
    for (uint32_t i = 0; i < entry_num; i++)
    {
        uint32_t* line_info_ptr = GET_FIRST_LINE_INFO_ADDRESS(entry_ptr);
        for (uint32_t j = 0; j < GET_LINE_INFO_LEN(entry_ptr); j++)
        {
            if (*(char*)GET_LINE_INFO_FUNCNAME_ADDRESS(line_info_ptr)!='\0')
            {
                funcname_ptr = (char*)GET_LINE_INFO_FUNCNAME_ADDRESS(line_info_ptr);
            }
            
            if(GET_INSTRUCTION_ADDRESS(line_info_ptr)==eip)
            {
                kernel_log(WARN, "filename:%s line:%u (%s)", (char*)POINT_AT_FILENAME(entry_ptr), 
                                                            GET_LINE_INFO_LINE_NUMBER(line_info_ptr)-1,
                                                            funcname_ptr);
                return 1;
            }
            else
            {
                line_info_ptr = GET_NEXT_ENTRY_ADDRESS(line_info_ptr);
            }
        }
        entry_ptr = line_info_ptr;
    }

    return 0;
}