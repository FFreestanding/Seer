#include <paging/vmm.h>
#include <paging/pde.h>
#include <paging/pte.h>
#include <paging/page.h>
#include <kernel_io/memory.h>
#include <paging/pmm.h>
#include <apic/cpu.h>
#include "common.h"

void *
vmm_map_page(uint32_t virtual_address, uint32_t physical_address)
{
    void *p = __vmm_map_page(virtual_address, physical_address,
                   DEFAULT_PAGE_FLAGS, DEFAULT_PAGE_FLAGS);
    if (p!=NULL_POINTER)
    {
        pmm_mark_chunk_occupied(physical_address>>12, 1);
    }
    return p;
}

void *
vmm_map_pages(uint32_t virtual_address, uint32_t physical_address, uint32_t counts)
{
    uint32_t addr1 = 0x1;
    uint32_t addr2 = 0x1;
    for (uint32_t i = 0; i < counts; ++i) {
        if (addr1 == 0x1) {
            addr1 = (uint32_t) vmm_map_page(virtual_address, physical_address);
            continue;
        }
        addr2 = (uint32_t) vmm_map_page(virtual_address + (i * 0x1000),
                                        physical_address + (i * 0x1000));
        if ((addr1+i*0x1000)==addr2) {
            continue;
        } else {
            return NULL_POINTER;
        }
    }
    return (void *) addr1;
}

void*
__vmm_map_page(uint32_t virtual_address, uint32_t physical_address,
               uint32_t directory_flags, uint32_t table_flags)
{
    if ((virtual_address == NULL_POINTER) || (physical_address == NULL_POINTER))
    {
        return NULL_POINTER;
    }
    
    uint32_t* page_directory_pointer =  (uint32_t*)PAGE_DIRECTORY_VIRTUAL_ADDRESS;
    uint32_t page_directory_offset = PDE_INDEX(virtual_address);

    uint32_t* page_table_pointer = PAGE_TABLE_VIRTUAL_ADDRESS(page_directory_offset);
    uint32_t page_table_offset = PTE_INDEX(virtual_address);
    
    /*
        Lunaixsky: 建议改成 < 1023， 清楚一点。 第 1023 条 PDE 是你的 pd_self_ref，不能被计入搜索范围！
    */

    while (page_directory_pointer[page_directory_offset] != NULL_POINTER 
        && page_directory_offset < 1023)
    {
        if (page_table_offset > 1023)
        {
            page_table_offset = 0;
            page_directory_offset++;

            /*
                Lunaixsky
            */
            page_table_pointer =  PAGE_TABLE_VIRTUAL_ADDRESS(page_directory_offset);
            continue;
        }

        /*
            Lunaixsky 复写非空页表？？
        */
        // if (page_table_pointer[page_table_offset] != NULL_POINTER)
        if (page_table_pointer[page_table_offset] == NULL_POINTER)
        {
            /*
                Lunaixsky: 我很想知道，PAGE_TABLE_SET 和 PAGE_TABLE_VM_SET的区别在哪里……
            */
            PAGE_TABLE_VM_SET(page_table_pointer[page_table_offset], 
                            physical_address, 
                            table_flags);
            return VIRTUAL_ADDRESS(page_directory_offset, page_table_offset, PAGE_OFFSET(virtual_address));
        }

        /*
            Lunaixsky: 死循环了！
        */
        page_table_offset++;
    }
    
    //找到新页目录项，但是没在之前的页目录项找到空页表项
    if (page_directory_offset >= 1023)
    {
        return NULL_POINTER;
    }
    
    // 页目录有空位，需要开辟一个新的 PDE
    uint8_t* new_pt_pa = pmm_alloc_page_entry();
    pmm_mark_chunk_occupied(((uint32_t)new_pt_pa) >> 12, 1);

    // 物理内存已满！
    if (!new_pt_pa) {
        return NULL_POINTER;
    }

    page_table_pointer =  PAGE_TABLE_VIRTUAL_ADDRESS(page_directory_offset);

    /*
        Lunaixsky: 所以，new_pt_pa 的分配只是个面子工程？
        Lunaixsky: page_directory_pointer[page_directory_offset] 已经是解引用了，
                   你往哪儿写页项呢！
    */

    // PAGE_DIRECTORY_SET(page_directory_pointer[page_directory_offset], 
    //                     PAGE_DIRECTORY_ADDRESS_32_12(virtual_address), 
    //                     directory_flags);

    PAGE_DIRECTORY_SET(&page_directory_pointer[page_directory_offset], 
                        new_pt_pa, 
                        directory_flags);

    // memory_set(PAGE_TABLE_VIRTUAL_ADDRESS(page_directory_offset), 0, 
    //             PAGE_TABLE_ENTRIES_SUM * PAGE_ENTRY_SIZE);

    memory_set((uint8_t*)page_table_pointer, 0, 
                 PAGE_TABLE_ENTRIES_SUM * PAGE_ENTRY_SIZE);

    /*
        Lunaixsky: PAGE_TABLE_SET宏的参数命名很让人疑惑，为什么第二个参数是virtual_address？
                   每个PTE的内容是 *物理地址*，对应的虚拟地址由该 PTE 在页表结构的层级位置蕴含！
    */

    // PAGE_TABLE_SET(page_table_pointer[page_table_offset], 
    //                 PAGE_DIRECTORY_ADDRESS_32_12(virtual_address), 
    //                 directory_flags);

    PAGE_TABLE_SET(&page_table_pointer[page_table_offset], 
                    physical_address, 
                    directory_flags);

    return (void*)VIRTUAL_ADDRESS(page_directory_offset, page_table_offset, PAGE_OFFSET(virtual_address));
}

void *
vmm_alloc_page_entry()
{
    uint32_t pp = (uint32_t)pmm_alloc_page_entry();
    void *result = vmm_map_page(pp, pp);
    if (result==NULL_POINTER) {
        vmm_unmap_page(pp);
        return NULL_POINTER;
    }
    pmm_mark_chunk_occupied(pp >> 12, 1);
    return result;
}

void
vmm_unmap_page(uint32_t virtual_address)
{
    if ((virtual_address & 0xFFF) != 0) { return; }
    uint32_t* page_table_ptr = (uint32_t*)
            ((uint32_t)PAGE_TABLE_VIRTUAL_ADDRESS(PDE_INDEX(virtual_address)) & 0xfffff000);

    pmm_mark_chunk_available(vaddr_to_paddr(virtual_address) >> 12, 1);
    if (page_table_ptr[PTE_INDEX(virtual_address)] != NULL)
    {
        page_table_ptr[PTE_INDEX(virtual_address)] = NULL;
        cpu_invplg((void *) virtual_address);
    }
}

void *
vmm_alloc_pages(uint32_t counts)
{
    uint32_t start = (uint32_t) pmm_alloc_pages(counts, 0);
    return (void *) vmm_map_pages(start, start, counts);
}