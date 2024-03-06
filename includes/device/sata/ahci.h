#ifndef SEER_AHCI_H
#define SEER_AHCI_H
#include <stdint.h>

#define AHCI_DEVICE_CLASS_CODE 0x10601

#define AHCI_IV 0x61

#define HBA_RGHC_ACHI_ENABLE (1 << 31)
#define HBA_RGHC_INTR_ENABLE (1 << 1)
#define HBA_RGHC_RESET 1

#define HBA_FIS_SIZE 256
#define HBA_CLB_SIZE 1024

#define HBA_RPxSSTS_IF(x) (((x) >> 4) & 0xf)

#define HBA_PxCMD_FRE (1 << 4)
#define HBA_PxCMD_CR (1 << 15)
#define HBA_PxCMD_FR (1 << 14)
#define HBA_PxCMD_ST (1)
#define HBA_PxINTR_DMA (1 << 2)
#define HBA_PxINTR_DHR (1)
#define HBA_PxINTR_DPS (1 << 5)
#define HBA_PxINTR_TFE (1 << 30)
#define HBA_PxINTR_HBF (1 << 29)
#define HBA_PxINTR_HBD (1 << 28)
#define HBA_PxINTR_IF (1 << 27)
#define HBA_PxINTR_NIF (1 << 26)
#define HBA_PxINTR_OF (1 << 24)
#define HBA_PxTFD_ERR (1)
#define HBA_PxTFD_BSY (1 << 7)
#define HBA_PxTFD_DRQ (1 << 3)

#define HBA_CMDH_FIS_LEN(fis_bytes) (((fis_bytes) / 4) & 0x1f)
#define HBA_CMDH_ATAPI (1 << 5)
#define HBA_CMDH_WRITE (1 << 6)
#define HBA_CMDH_PREFETCH (1 << 7)
#define HBA_CMDH_R (1 << 8)
#define HBA_CMDH_CLR_BUSY (1 << 10)
#define HBA_CMDH_PRDT_LEN(entries) (((entries)&0xffff) << 16)

#define HBA_MY_IE (HBA_PxINTR_DHR | HBA_PxINTR_TFE | HBA_PxINTR_OF)

#define HBA_DEV_FEXTLBA 1

// PRDT: Physical Region Descriptor Table
struct prdte {
    uint32_t data_base;
    uint32_t reserved[2];
    uint32_t byte_count;
} __attribute__((packed));

struct command_table {
    uint8_t command_fis[0x40];
    uint8_t atapi_cmd[0x10];
    uint8_t reserved[0x30];
    struct prdte entries[4];
} __attribute__((packed));

struct command_header {
    uint16_t options;
    uint16_t prdt_len;
    uint32_t byte_count;
    uint32_t cmd_table_base;
    uint32_t reserved[5];
} __attribute__((packed));

struct command_list {
    struct command_header command[32];
} __attribute__((packed));

struct ahci_port_registers {
    struct command_list *pxclb;
    uint32_t pxclbu;
    uint32_t pxfb;
    uint32_t pxfbu;
    uint32_t pxis;
    uint32_t pxie;
    uint32_t pxcmd;
    uint32_t reserved1;
    uint32_t pxtfd;
    uint32_t pxsig;
    uint32_t pxssts;
    uint32_t pxsctl;
    uint32_t pxserr;
    uint32_t pxsact;
    uint32_t pxci;
    uint32_t pxsntf;
    uint32_t pxfbs;
    uint32_t pxdevslp;
    uint32_t reserved2[10];
    uint32_t pxvs[4];
} __attribute__((packed));

struct ahci_registers {
    // global registers
    uint32_t cap;
    uint32_t ghc;
    uint32_t is;
    uint32_t pi;
    uint32_t vs;
    uint32_t ccc_ctl;
    uint32_t ccc_ports;
    uint32_t em_loc;
    uint32_t em_ctl;
    uint32_t cap2;
    uint32_t bohc;
    uint8_t reserved[212];
    // port registers
    struct ahci_port_registers port[32];
} __attribute__((packed));

struct ahci_device {
    struct ahci_registers *ahci_regs;
    uint32_t port_num;
    uint32_t cmd_slots_num;
    uint32_t version;
    uint32_t port_bitmap;
} __attribute__((packed));

struct ahci_device_info {
    char serial_num[20];
    char model[40];
    uint32_t flags;
    uint64_t max_lba;
    uint32_t block_size;
    uint64_t wwn;
    uint32_t alignment_offset;
    uint32_t block_per_sec;
    uint32_t capabilities;
    uint8_t cbd_size;
};


void ahci_device_init();

uint32_t ahci_sizing_addr_size();

void config_per_port();

void reset_port(struct ahci_port_registers *port_reg);

void probe_disk_info(struct ahci_port_registers* port_reg);

int ahci_prepare_cmd(struct ahci_port_registers* port_reg, struct command_table *table);

int __get_free_slot(struct ahci_port_registers* port_reg);

int ahci_try_send(struct ahci_port_registers* port, int slot);

void ahci_parse_dev_info(struct ahci_device_info* info, uint16_t *data);

void ahci_parsestr(char* str, uint16_t *reg_start, int size_word);

#endif //SEER_AHCI_H
