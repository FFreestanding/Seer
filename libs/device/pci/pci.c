#include <device/pci/pci.h>
#include <apic/apic.h>
#include <kernel_io/heap.h>


static struct pci_device_manager pci_dev_mgr;

void init_pci_device_manager()
{
    llist_init_head(&pci_dev_mgr.pci_device_head);
}

struct pci_device_manager *get_pci_device_manager()
{
    return &pci_dev_mgr;
}

void prob_pci_device()
{
    struct pci_address addr = {
            .reversed = 0x80,
            .bus = 0,
            .dev = 0,
            .function = 0,
            .reg = 0
    };
    for (; addr.bus <= 255; ++addr.bus) {
        for (; addr.dev <= 31; ++addr.dev) {
            __prob_pci_device(&addr);
            if (addr.dev == 31){break;}
        }
        if (addr.bus == 255){break;}
    }
}

void pci_set_up_msi()
{
    struct pci_device *pos, *next;
    llist_for_each(pos, next, &pci_dev_mgr.pci_device_head, other_pci_devices)
    {
        __pci_set_up_msi(pos, MIS_IV);
    }
}

void __pci_set_up_msi(struct pci_device *device, int interrupt_vector)
{
    uint16_t status = pci_read_status((const uint32_t *) &device->address);
    if (!(status&0x10))
    {
        device->capabilities_pointer = 0;
        return;
    }

    uint8_t cap_p = pci_read_capabilities_pointer((uint32_t *) &device->address);
    while (cap_p) {
        device->address.reg = cap_p;
        uint32_t cap_hdr = pci_read_config_space((uint32_t *) &device->address);
        if ((cap_p & 0xff) == 0x5) {
            device->capabilities_pointer = cap_p;
            break;
        }
        cap_p = (uint8_t)(cap_hdr >> 8);
    }

    device->address.reg = device->capabilities_pointer + 0x4;
    pci_write_config_space((uint32_t *) &device->address,
                           APIC_BASE_PHYSICAL_ADDRESS | 0x8);

    device->address.reg = device->capabilities_pointer + 0x8;
    pci_write_config_space((uint32_t *) &device->address,
                           interrupt_vector);
}

void __prob_pci_device(struct pci_address *addr)
{
    uint16_t vendor_id = pci_read_vendor_id((const uint32_t *) addr);
    if (vendor_id == 0xffff)
    {
        return;
    }
    uint8_t header_type = pci_read_header_type((const uint32_t *) addr);
    if (header_type & 0b10000000 && addr->function == 0)
    {
        for (addr->function = 1; addr->function < 7; ++addr->function) {
            __prob_pci_device(addr);
        }
        addr->function = 0;
    }

    struct pci_device *dev = (struct pci_device *) kmalloc(sizeof(struct pci_device));
    dev->address = *addr;
    dev->capabilities_pointer = 0;
    dev->class_code = pci_read_class_code(addr);
    llist_init_head(&dev->other_pci_devices);

    llist_append(&pci_dev_mgr.pci_device_head, &dev->other_pci_devices);
}
