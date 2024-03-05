#ifndef SEER_PCI_H
#define SEER_PCI_H
#include <stdint.h>
#include <apic/cpu.h>
#include <kernel_io/memory.h>
#include <data_structure/llist.h>

#define PCI_ADDRESS_PORT 0xcf8
#define PCI_REG_PORT 0xcfc

#define MIS_IV 0x66

struct pci_address {
    uint8_t reg;//offset of config space
    uint8_t function:3;
    uint8_t dev:5;
    uint8_t bus;
    uint8_t reversed;
} __attribute__((packed));

struct pci_device {
    struct pci_address address;
    uint8_t capabilities_pointer;
    struct llist_header other_pci_devices;
};

struct pci_device_manager
{
    struct pci_device* pci_device_head;
};

struct configuration_space_header {
    uint16_t vendor_id;
    uint16_t device_id;
    uint16_t command;
    uint16_t status;
    uint8_t revision_id;
    uint8_t class_code[24];
    uint8_t cacheline_size;
    uint8_t latency_timer;
    uint8_t header_type;
    uint8_t bist;
    uint32_t base_address_registers[6];
    uint32_t cardbus_cis_pointer;
    uint16_t subsystem_vendor_id;
    uint16_t subsystem_id;
    uint32_t expansion_rom_base_address;
    uint16_t capabilities_pointer;
    uint16_t reserved1;
    uint32_t reserved2;
    uint8_t interrupt_line;
    uint8_t interrupt_pin;
    uint8_t min_gnt;
    uint8_t max_lat;
} __attribute__((packed));

void init_pci_device_manager();

struct pci_device_manager *get_pci_device_manager();

void prob_pci_device();

void __prob_pci_device(struct pci_address *addr);

void pci_set_up_msi();

void __pci_set_up_msi(struct pci_device *device, int interrupt_vector);

static inline uint32_t pci_read_config_space(const uint32_t *addr)
{
    io_outl(PCI_ADDRESS_PORT, *addr);
    return io_inl(PCI_REG_PORT);
}

static inline void pci_write_config_space(const uint32_t *addr, uint32_t data)
{
    io_outl(PCI_ADDRESS_PORT, *addr);
    io_outl(PCI_REG_PORT, data);
}

static inline uint16_t pci_read_vendor_id(const uint32_t *addr)
{
    struct pci_address *p = (struct pci_address *)addr;
    p->reg = 0;
    return (uint16_t)pci_read_config_space(addr);
}

static inline uint16_t pci_read_device_id(const uint32_t *addr)
{
    struct pci_address *p = (struct pci_address *)addr;
    p->reg = 2;
    return (uint16_t)pci_read_config_space(addr);
}

static inline uint16_t pci_read_command(const uint32_t *addr)
{
    struct pci_address *p = (struct pci_address *)addr;
    p->reg = 4;
    return (uint16_t)pci_read_config_space(addr);
}

static inline uint16_t pci_read_status(const uint32_t *addr)
{
    struct pci_address *p = (struct pci_address *)addr;
    p->reg = 6;
    return (uint16_t)pci_read_config_space(addr);
}

static inline uint32_t pci_read_class_code(const uint32_t *addr)
{
    struct pci_address *p = (struct pci_address *)addr;
    p->reg = 8;
    return (uint32_t)(pci_read_config_space(addr)>>8);
}

static inline uint8_t pci_read_header_type(const uint32_t *addr)
{
    struct pci_address *p = (struct pci_address *)addr;
    p->reg = 0xe;
    return (uint8_t)pci_read_config_space(addr);
}

static inline uint8_t pci_read_capabilities_pointer(const uint32_t *addr)
{
    struct pci_address *p = (struct pci_address *)addr;
    p->reg = 0x34;
    return (uint8_t)pci_read_config_space(addr);
}

static inline void print_pci_dev_info(struct pci_address *addr)
{
    static uint32_t i = 0;
    if (i==10){ while (1){}}
    else{i++;}
    kernel_log(INFO, "bus:%h dev:%h func:%h  DeviceID:%h VendorID:%h ClassCode:%h",
               addr->bus, addr->dev, addr->function,
               pci_read_device_id((const uint32_t *) addr),
               pci_read_vendor_id((const uint32_t *) addr),
               pci_read_class_code((const uint32_t *) addr));
}

#endif //SEER_PCI_H
