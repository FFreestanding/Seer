#ifndef __PTE_H
#define __PTE_H 1

/*--- Structure of Page Table Entry ---*/

//0 P:Present
#define PAGE_TABLE_PRESENT(x)   (x & 0b1)
//1 R/W:Read/Write
#define PAGE_TABLE_READ_WRITE(x)   ((x & 0b1) << 1)
//2 U/S:User/Supervisor
#define PAGE_TABLE_USER_SUPERVISOR(x)   ((x & 0b1) << 2)
//3 PWT:Write-Through
#define PAGE_TABLE_WRITE_THROUGH(x)   ((x & 0b1) << 3)
//4 PCD:Cache Disable
#define PAGE_TABLE_CACHE_DISABLE(x)   ((x & 0b1) << 4)
//5 A:Accessed
#define PAGE_TABLE_ACCESSED(x)   ((x & 0b1) << 5)
//6 D:Dirty
#define PAGE_TABLE_DIRTY(x)   ((x & 0b1) << 6)
//7 PAT:Page Attribute Table
#define PAGE_TABLE_PAGE_ATTRIBUTE_TABLE(x)   ((x & 0b1) << 7)
//8 G:Global
#define PAGE_TABLE_GLOVAL(x)   ((x & 0b1) << 8)
//9-11 AVL:Available
#define PAGE_TABLE_AVAILAVLE(x)   (((x) & 0b111) << 9)
//12-31 Bits 31-12 of address
#define PAGE_TABLE_ADDRESS_32_12(x) ((uint32_t)(x) & 0xfffff000)

#define PAGE_TABLE_SET(pte_address, virtual_address, flags) *((uint32_t*)pte_address) = (virtual_address) | (flags)
#define PAGE_TABLE_VM_SET(pte_address, virtual_address, flags) (pte_address) = (virtual_address) | (flags)
#define PTE_INDEX(address) (((address) & 0x003ff000) >> 12)
#define PAGE_TABLE_ENTRIES_SUM 1024

#endif