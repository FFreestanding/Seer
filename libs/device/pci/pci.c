#include <device/pci/pci.h>
#include <apic/apic.h>
#include <kernel_io/heap.h>

static struct llist_header pci_devices;

void
pci_probe_msi_info(struct pci_device* device);

void
pci_probe_device(int bus, int dev, int funct)
{
    uint32_t base = PCI_ADDRESS(bus, dev, funct);
    pci_reg_t reg1 = pci_read_cspace(base, 0);

    // Vendor=0xffff则表示设备不存在
    if (PCI_DEV_VENDOR(reg1) == PCI_VENDOR_INVLD) {
        return;
    }

    pci_reg_t hdr_type = pci_read_cspace(base, 0xc);
    hdr_type = (hdr_type >> 16) & 0xff;

    // 防止堆栈溢出
    // QEMU的ICH9/Q35实现似乎有点问题，对于多功能设备的每一个功能的header type
    //  都将第七位置位。而virtualbox 就没有这个毛病。
    if ((hdr_type & 0x80) && funct == 0) {
        hdr_type = hdr_type & ~0x80;
        // 探测多用途设备（multi-function device）
        for (int i = 1; i < 7; i++) {
            pci_probe_device(bus, dev, i);
        }
    }

    if (hdr_type != PCI_TDEV) {
        // XXX: 目前忽略所有桥接设备，比如PCI-PCI桥接器，或者是CardBus桥接器
        return;
    }

    pci_reg_t intr = pci_read_cspace(base, 0x3c);
    pci_reg_t class = pci_read_cspace(base, 0x8);

    struct pci_device* device = kmalloc(sizeof(struct pci_device));
    *device = (struct pci_device){ .cspace_base = base,
            .class_info = class,
            .device_info = reg1,
            .intr_info = intr };

    pci_probe_msi_info(device);

    llist_append(&pci_devices, &device->dev_chain);
}

void
pci_probe()
{
    // 暴力扫描所有PCI设备
    // XXX: 尽管最多会有256条PCI总线，但就目前而言，只考虑bus #0就足够了
    for (int bus = 0; bus < 1; bus++) {
        for (int dev = 0; dev < 32; dev++) {
            pci_probe_device(bus, dev, 0);
        }
    }
}

void
pci_probe_msi_info(struct pci_device* device)
{
    // Note that Virtualbox have to use ICH9 chipset for MSI support.
    // Qemu seems ok with default PIIX3, Bochs is pending to test...
    //    See https://www.virtualbox.org/manual/ch03.html (section 3.5.1)
    pci_reg_t status =
            pci_read_cspace(device->cspace_base, PCI_REG_STATUS_CMD) >> 16;

    if (!(status & 0x10)) {
        device->msi_loc = 0;
        return;
    }

    pci_reg_t cap_ptr = pci_read_cspace(device->cspace_base, 0x34) & 0xff;
    uint32_t cap_hdr;

    while (cap_ptr) {
        cap_hdr = pci_read_cspace(device->cspace_base, cap_ptr);
        if ((cap_hdr & 0xff) == 0x5) {
            // MSI
            device->msi_loc = cap_ptr;
            return;
        }
        cap_ptr = (cap_hdr >> 8) & 0xff;
    }
}

#define PCI_PRINT_BAR_LISTING

void
pci_print_device()
{
    struct pci_device *pos, *n;
    llist_for_each(pos, n, &pci_devices, dev_chain)
    {
        kernel_log(INFO, "(B%hh:D%hh:F%hh) Dev %h:%h, Class 0x%h",
                PCI_BUS_NUM(pos->cspace_base),
                PCI_SLOT_NUM(pos->cspace_base),
                PCI_FUNCT_NUM(pos->cspace_base),
                PCI_DEV_VENDOR(pos->device_info),
                PCI_DEV_DEVID(pos->device_info),
                PCI_DEV_CLASS(pos->class_info));

        kernel_log(INFO, "\t IRQ: %u, INT#x: %u",
                PCI_INTR_IRQ(pos->intr_info),
                PCI_INTR_PIN(pos->intr_info));
#ifdef PCI_PRINT_BAR_LISTING
        pci_reg_t bar;
        for (size_t i = 1; i <= 6; i++) {
            size_t size = pci_bar_sizing(pos, &bar, i);

            if (!bar)
                continue;
            if (PCI_BAR_MMIO(bar)) {
                kernel_log(INFO, "\t BAR#%u (MMIO) %h [%u]",
                        i,
                        PCI_BAR_ADDR_MM(bar),
                        size);
            } else {
                kernel_log(INFO, "\t BAR#%u (I/O) %h [%u]",
                        i,
                        PCI_BAR_ADDR_IO(bar),
                        size);
            }
        }
#endif
        if (pos->msi_loc) {
            kernel_log(INFO, "\t MSI supported (@%hh)", pos->msi_loc);
        }
    }
}

size_t
pci_bar_sizing(struct pci_device* dev, uint32_t* bar_out, uint32_t bar_num)
{
    pci_reg_t bar = pci_read_cspace(dev->cspace_base, PCI_REG_BAR(bar_num));
    if (!bar) {
        *bar_out = 0;
        return 0;
    }

    pci_write_cspace(dev->cspace_base, PCI_REG_BAR(bar_num), 0xffffffff);
    pci_reg_t sized =
            pci_read_cspace(dev->cspace_base, PCI_REG_BAR(bar_num)) & ~0x1;
    if (PCI_BAR_MMIO(bar)) {
        sized = PCI_BAR_ADDR_MM(sized);
    }
    *bar_out = bar;
    pci_write_cspace(dev->cspace_base, PCI_REG_BAR(bar_num), bar);
    return ~sized + 1;
}

void
pci_setup_msi(struct pci_device* device, int vector)
{
    // Dest: APIC#0, Physical Destination, No redirection
    uint32_t msi_addr = APIC_BASE_PHYSICAL_ADDRESS;

    // Edge trigger, Fixed delivery
    uint32_t msi_data = vector;

    pci_write_cspace(
            device->cspace_base, PCI_MSI_ADDR(device->msi_loc), msi_addr);
    pci_write_cspace(
            device->cspace_base, PCI_MSI_DATA(device->msi_loc), msi_data & 0xffff);

    pci_reg_t reg1 = pci_read_cspace(device->cspace_base, device->msi_loc);

    // manipulate the MSI_CTRL to allow device using MSI to request service.
    reg1 = ((((reg1 >> 16) & ~0x70) | 0x1) << 16) | (reg1 & 0xffff);
    pci_write_cspace(device->cspace_base, device->msi_loc, reg1);
}

struct pci_device*
pci_get_device_by_id(uint16_t vendorId, uint16_t deviceId)
{
    uint32_t dev_info = vendorId | (deviceId << 16);
    struct pci_device *pos, *n;
    llist_for_each(pos, n, &pci_devices, dev_chain)
    {
        if (pos->device_info == dev_info) {
            return pos;
        }
    }

    return NULL;
}

struct pci_device*
pci_get_device_by_class(uint32_t class)
{
    struct pci_device *pos, *n;
    llist_for_each(pos, n, &pci_devices, dev_chain)
    {
        if (PCI_DEV_CLASS(pos->class_info) == class) {
            return pos;
        }
    }

    return NULL;
}

void
pci_init()
{
    llist_init_head(&pci_devices);
    // Otherwise, fallback to use legacy PCI 3.0 method.
    pci_probe();
}

/*static struct pci_device_manager pci_dev_mgr;

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
    // Dest: APIC#0, Physical Destination, No redirection
    uint32_t msi_addr = APIC_BASE_PHYSICAL_ADDRESS;

    // Edge trigger, Fixed delivery
    uint32_t msi_data = interrupt_vector;

    device->address.reg = device->capabilities_pointer + 0x4;
    pci_write_config_space((uint32_t *) &device->address,msi_addr);

    device->address.reg = device->capabilities_pointer + 0x8;
    pci_write_config_space((uint32_t *) &device->address,msi_data);

    device->address.reg = device->capabilities_pointer;
    uint32_t reg1 = pci_read_config_space((uint32_t *) &device->address);

    // manipulate the MSI_CTRL to allow device using MSI to request service.
    reg1 = ((((reg1 >> 16) & ~0x70) | 0x1) << 16) | (reg1 & 0xffff);
    pci_write_config_space((uint32_t *) &device->address, reg1);

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

    // set capabilities_pointer
    uint16_t status = pci_read_status((const uint32_t *) &dev->address);
    if (!(status&0x10))
    {
        dev->capabilities_pointer = 0;
        return;
    }

    uint8_t cap_p = pci_read_capabilities_pointer((uint32_t *) &dev->address);
    while (cap_p) {
        dev->address.reg = cap_p;
        uint32_t cap_hdr = pci_read_config_space((uint32_t *) &dev->address);
        if ((cap_hdr & 0xff) == 0x5) {
            dev->capabilities_pointer = cap_p;
            break;
        }
        cap_p = (uint8_t)(cap_hdr >> 8);
    }

}*/
