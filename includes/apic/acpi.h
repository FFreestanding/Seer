#ifndef __ACPI_H
#define __ACPI_H 1

#include <stdint.h>
#include <boot/multiboot.h>

#define ACPI_RSDP_SIG_L       0x20445352      // 'RSD '
#define ACPI_MADT_SIG        0x43495041       // 'APIC'
#define ACPI_FADT_SIG        0x50434146       // 'FACP'
#define ACPI_MADT_LAPIC 0x0  // Local APIC
#define ACPI_MADT_IOAPIC 0x1 // I/O APIC
#define ACPI_MADT_INTSO 0x2  // Interrupt Source Override

#define IAPC_ARCH_LEGACY   0x1
#define IAPC_ARCH_8042     0x2
#define IAPC_ARCH_NO_VGA   0x4
#define IAPC_ARCH_NO_MSI   0x8
#define IAPC_ARCH_ASPM     0x10
#define IAPC_ARCH_NO_RTC   0x20



#pragma pack(1)
typedef struct
{
    uint32_t signature;
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oem_id[6];
    char oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
} acpi_header_t;
#pragma pack()

/* Fixed ACPI Description Table (FADT) */
// https://uefi.org/htmlspecs/ACPI_Spec_6_4_html/05_ACPI_Software_Programming_Model/ACPI_Software_Programming_Model.html#fadt-format
#pragma pack(1)
typedef struct
{
    uint32_t signature;
    uint32_t length;
    uint8_t fadt_major_version;
    uint8_t checksum;
    char oem_id[6];
    char oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
    uint32_t firmware_ctrl;
    uint32_t dsdt;
    uint8_t reserved;
    uint8_t preferred_pm_profile;
    uint8_t sci_int[2];
    uint32_t smi_cmd;
    uint8_t acpi_enable;
    uint8_t acpi_disable;
    uint8_t s4bios_req;
    uint8_t pstate_cnt;
    uint32_t pm1a_evt_blk;
    uint32_t pm1b_evt_blk;
    uint32_t pm1a_cnt_blk;
    uint32_t pm1b_cnt_blk;
    uint32_t pm2_cnt_blk;
    uint32_t pm_tmr_blk;
    uint32_t gpe0_blk;
    uint32_t gpe1_blk;
    uint8_t pm1_evt_len;
    uint8_t pm1_cnt_len;
    uint8_t pm2_cnt_len;
    uint8_t pm_tmr_len;
    uint8_t gpe0_blk_len;
    uint8_t gpe1_blk_len;
    uint8_t gpe1_base;
    uint8_t cst_cnt;
    uint16_t p_lvl2_lat;
    uint16_t p_lvl3_lat;
    uint16_t flush_size;
    uint16_t flush_stride;
    uint8_t duty_offset;
    uint8_t duty_width;
    uint8_t day_alrm;
    uint8_t mon_alrm;
    uint8_t century;
    uint16_t iapc_boot_arch;
    uint8_t flags[268+8-(109+2)];
} acpi_fadt_t;
#pragma pack()

/* Processor Local APIC Structure */
// https://uefi.org/htmlspecs/ACPI_Spec_6_4_html/05_ACPI_Software_Programming_Model/ACPI_Software_Programming_Model.html#processor-local-apic-structure-1
#pragma pack(1)
typedef struct
{
    uint8_t type;
    uint8_t length;
    uint8_t acpi_processor_uid;
    uint8_t apic_id;
    uint32_t flags;
} acpi_lapic_t;
#pragma pack()

/* Root System Description Table (RSDT) */
// https://uefi.org/htmlspecs/ACPI_Spec_6_4_html/05_ACPI_Software_Programming_Model/ACPI_Software_Programming_Model.html#root-system-description-table-fields-rsdt
#pragma pack(1)
typedef struct
{
    acpi_header_t header;
    acpi_header_t* entry;
} acpi_rsdt_t;
#pragma pack()

/* Extended System Description Table Fields (XSDT) */
// https://uefi.org/htmlspecs/ACPI_Spec_6_4_html/05_ACPI_Software_Programming_Model/ACPI_Software_Programming_Model.html#extended-system-description-table-fields-xsdt
#pragma pack(1)
typedef struct
{
    acpi_header_t header;
    acpi_header_t* entry;
} acpi_xsdt_t;
#pragma pack()



/* Multiple APIC Description Table (MADT) Format */
// https://uefi.org/htmlspecs/ACPI_Spec_6_4_html/05_ACPI_Software_Programming_Model/ACPI_Software_Programming_Model.html#multiple-apic-description-table-madt-format
#pragma pack(1)
typedef struct
{
    acpi_header_t header;
    uint32_t* local_interrupt_controller_address;
    uint32_t flags;
    // uint32_t* local_interrupt_controller;
} acpi_madt_t;
#pragma pack()

/* I/O APIC Structure */
// https://uefi.org/htmlspecs/ACPI_Spec_6_4_html/05_ACPI_Software_Programming_Model/ACPI_Software_Programming_Model.html#io-apic-structure
#pragma pack(1)
typedef struct
{
    uint8_t type;
    uint8_t length;
    uint8_t ioapic_id;
    uint8_t reserved;
    uint32_t ioapic_address;
    uint32_t global_system_interrupt_base;
} acpi_ioapic_t;
#pragma pack()

/* Interrupt Source Override Structure */
// https://uefi.org/htmlspecs/ACPI_Spec_6_4_html/05_ACPI_Software_Programming_Model/ACPI_Software_Programming_Model.html#interrupt-source-override-structure
#pragma pack(1)
typedef struct
{
    uint8_t type;
    uint8_t length;
    uint8_t bus;
    uint8_t source;
    uint32_t global_system_interrupt;
    uint8_t flags[2];
} acpi_isos_t;
#pragma pack()

#pragma pack(1)
typedef struct
{
    // Make it as null terminated
    char oem_id[7];
    uint32_t* lapic_address;
    acpi_lapic_t* lapic;
    acpi_ioapic_t* ioapic;
    acpi_isos_t** irq;
    acpi_fadt_t fadt;
} acpi_context;
#pragma pack()

#pragma pack(1)
typedef struct
{
    void* apic_addr;
    acpi_lapic_t* apic;
    acpi_ioapic_t* ioapic;
    acpi_isos_t** irq_exception;
} acpi_madt_toc_t;
#pragma pack()


#pragma pack(1)
typedef struct
{
    uint8_t type;
    uint8_t length;
} acpi_interrupt_controller_struct;

// https://uefi.org/htmlspecs/ACPI_Spec_6_4_html/05_ACPI_Software_Programming_Model/ACPI_Software_Programming_Model.html#root-system-description-pointer-rsdp-structure
// Root System Description Pointer (RSDP)
#pragma pack(1)
typedef struct {
    char signature[8];
    uint8_t chksum;
    char oem_id[6];
    uint8_t revision;
    acpi_rsdt_t* rsdt;
    uint32_t length;
    acpi_xsdt_t* xsdt;
    uint8_t extended_checksum;
    char reserved[3];
} acpi_rsdp_t;
#pragma pack()



void 
_acpi_init(multiboot_info_t* mb_info);

acpi_rsdp_t*
acpi_locate_rsdp(multiboot_info_t* mb_info);

uint8_t
acpi_rsdp_validate(acpi_rsdp_t* rsdp);

acpi_context*
get_acpi_context_instance();

void
madt_parse(acpi_madt_t* madt, acpi_context* toc);


#endif