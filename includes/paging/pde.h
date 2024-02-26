#ifndef __PDE_H
#define __PDE_H 1

/*--- Structure of Page Directory Entry (PS=0) ---
Page size (PS) flag, bit 7 page-directory entries for 4-KByte pages Determines the page size. 
When this flag is clear, the page size is 4 KBytes and the page-directory entry points to a page table. 
When the flag is set, the page size is 4 MBytes for normal 32-bit addressing 
(and 2 MBytes if extended physical addressing is enabled) and the page- directory entry points to a page. 
If the page-directory entry points to a page table, all the pages associated with that page table will be 4-KByte pages.
*/

//0 P:Present
#define PAGE_DIRECTORY_PRESENT(x)   (x & 0b1)
//1 R/W:Read/Write
#define PAGE_DIRECTORY_READ_WRITE(x)   ((x & 0b1) << 1)
//2 U/S:User/Supervisor
#define PAGE_DIRECTORY_USER_SUPERVISOR(x)   ((x & 0b1) << 2)
//3 PWT:Write-Through
#define PAGE_DIRECTORY_WRITE_THROUGH(x)   ((x & 0b1) << 3)
//4 PCD:Cache Disable
#define PAGE_DIRECTORY_CACHE_DISABLE(x)   ((x & 0b1) << 4)
//5 A:Accessed
#define PAGE_DIRECTORY_ACCESSED(x)   ((x & 0b1) << 5)
/*6 AVL:Available
Bits 9 through 11 (if PS=0, also bits 6 & 8) are not used by the processor, and are free for the OS to store some of its own accounting information.*/
#define PAGE_DIRECTORY_AVAILAVLE_6(x)   ((x & 0b1) << 6)
/*7 PS:Page Size
Page Size stores the page size for that specific entry. If the bit is set, then the PDE maps to a page that is 4 MiB in size. Otherwise, it maps to a 4 KiB page table. 
*/
#define PAGE_DIRECTORY_PAGE_ATTRIBUTE_TABLE (0 << 7)
/*8-11 AVL:Available
Bits 9 through 11 (if PS=0, also bits 6 & 8) are not used by the processor, and are free for the OS to store some of its own accounting information.*/
#define PAGE_DIRECTORY_AVAILAVLE_11_8(x)   ((x & 0b111) << 8)
//12-31 Bits 31-12 of address
#define PAGE_DIRECTORY_ADDRESS_32_12(x) ((uint32_t)(x) & 0xfffff000)

#define PAGE_DIRECTORY_SET(pde_address, virtual_address, flags) *((uint32_t*)(pde_address)) = ((uint32_t)(virtual_address)) | (flags)

#define PDE_INDEX(address) (((address) & 0xffc00000) >> 22)

#define PAGE_DIRECTORY_VIRTUAL_ADDRESS 0xfffff000

#endif

