#include <device/sata/ahci.h>
#include <device/pci/pci.h>
#include <paging/vmm.h>
#include <paging/pmm.h>
#include <paging/page.h>
#include <idt/interrupts.h>
#include <kernel_io/heap.h>
#include <device/sata/sata.h>
#include <device/sata/disk/ata/ata.h>
#include <device/sata/disk/atapi/scsi.h>

static struct ahci_device *main_ahci_dev;

struct ahci_device *
get_main_ahci_dev()
{
    return main_ahci_dev;
}

void
ahci_isr(isr_param* p)
{
    kernel_log(INFO, "AHCI ISR");
}

void ahci_device_init()
{
    main_ahci_dev = kmalloc(sizeof(struct ahci_device));

    struct pci_device_manager *pci_dev_mgr = get_pci_device_manager();
    struct pci_device *pos, *next;

    llist_for_each(pos, next, &pci_dev_mgr->pci_device_head, other_pci_devices)
    {
        if (pos->class_code == AHCI_DEVICE_CLASS_CODE)
        {
            kernel_log(INFO, "FOUND AHCI Device");
            print_pci_dev_info(pos);
            break;
        }
    }

    uint16_t cmd = pci_read_command((uint32_t *)&pos->address);
    // 使用MSI 启用MMIO访问
    cmd |= (PCI_RCMD_MM_ACCESS | PCI_RCMD_DISABLE_INTR | PCI_RCMD_BUS_MASTER);
    pci_write_command(&pos->address, cmd);
    __pci_set_up_msi(pos, AHCI_IV);

    interrupt_routine_subscribe(AHCI_IV, ahci_isr);

    uint32_t start = pci_read_bar6(&pos->address);
    uint32_t size = ahci_sizing_addr_size(pos)/0x1000;

    struct ahci_registers *ahci_regs = vmm_map_page(0xc015d000, start,
                  DEFAULT_PAGE_FLAGS,DEFAULT_PAGE_FLAGS);

    if (ahci_regs==NULL_POINTER) {
        kernel_log(ERROR, "Failed to map AHCI Register Space | Size:%h", size);
    }

    main_ahci_dev->ahci_regs = ahci_regs;

    // Enable AHCI
    // AHCI Enable (AE): When set, indicates that
    // communication to the HBA shall be via AHCI mechanisms.
    ahci_regs->ghc |= HBA_RGHC_ACHI_ENABLE;
    // Interrupt Enable (IE): This global bit enables interrupts from the HBA.
    ahci_regs->ghc |= HBA_RGHC_INTR_ENABLE;

    uint32_t cap = ahci_regs->cap;

    main_ahci_dev->port_num = (cap&0x1f)+1; // CAP.NP
    main_ahci_dev->cmd_slots_num = (cap>>8)&0x1f; // CAP.NCS
    main_ahci_dev->version = main_ahci_dev->ahci_regs->vs;
    main_ahci_dev->port_bitmap = ahci_regs->pi;

    config_per_port();

}

uint32_t ahci_sizing_addr_size(struct pci_device* device, uint32_t bar6)
{
    // Software saves the original value of the Base Address register
    // bar6 will save it


    // writes 0 FFFF FFFFh to the register,
    pci_write_config_space(device, 0xffffffff);
    // then reads it back.
    uint32_t sized = pci_read_bar6(device) & ~0x1;

    // Note that the upper 16 bits of the result is ignored
    // if the Base Address register is for I/O and bits 16-31 returned zero upon read.
    // 这个由硬件实现，软件不用关心

    // Size calculation can be done from the
    // 32-bit value read by first clearing encoding information bits
    // (bit 0 for I/O, bits 0-3 for memory)
    if (PCI_BAR_MMIO(bar6)) {
        sized = PCI_BAR_ADDR_MM(sized);
    }

    // 恢复基地址
    pci_write_bar6(device, bar6);
    // inverting all 32 bits (logical NOT), then incrementing by 1.
    return ~sized + 1;
}

void config_per_port()
{
    uint32_t bitmap = main_ahci_dev->port_bitmap;
    uint32_t clb_pg_addr = 0, fis_pg_addr = 0;
    uint32_t clb_pa = 0, fis_pa = 0;

    // 对于每一个端口
    for (size_t i = 0, fisp = 0, clbp = 0; i < 32;
         i++, bitmap >>= 1, fisp = (fisp + 1) % 16, clbp = (clbp + 1) % 4) {
        if (!(bitmap & 0x1)) {
            continue;
        }

        // 获得每一个端口
        struct ahci_port_registers *reg = &main_ahci_dev->ahci_regs->port[i];

        // 3.1 进行端口重置
        reset_port(reg);

        // 3.2 分配操作空间，设置 PxCLB 和 PxFB
        if (!clbp) {
            // 每页最多4个命令队列
            clb_pa = (uint32_t) pmm_alloc_page_entry();
            clb_pg_addr = (uint32_t) vmm_alloc_page_entry((void *) 0x8000000,
                                                          DEFAULT_PAGE_FLAGS,
                                                          DEFAULT_PAGE_FLAGS);
            memory_set_fast((void*)clb_pg_addr, 0, 0x1000/4);
        }
        if (!fisp) {
            // 每页最多16个FIS
            fis_pa = (uint32_t) pmm_alloc_page_entry();
            fis_pg_addr = (uint32_t) vmm_alloc_page_entry((void *) 0x9000000,
                                                          DEFAULT_PAGE_FLAGS,
                                                          DEFAULT_PAGE_FLAGS);
            memory_set_fast((void*)fis_pg_addr, 0, 0x1000/4);
        }

        /* 重定向CLB与FIS（设置寄存器） */
        reg->pxclb = (struct command_list *) (clb_pa + clbp * HBA_CLB_SIZE);
        reg->pxfb = fis_pa + fisp * HBA_FIS_SIZE;

        /* 初始化端口，并置于就绪状态 */
        reg->pxci = 0;
        // 3.3 将 PxSERR 寄存器清空
        reg->pxserr = -1;

        // Current Interface Speed (SPD):
        // Indicates the negotiated interface communication speed.
        // Device not present or communication not established
        if (!HBA_RPxSSTS_IF(reg->pxssts)) {
            continue;
        }

        // 确保当前端口的 DMA 控制器处在关闭状态
        // 然后再通过 FRE 和 ST 置位，开启 Port
        while (reg->pxcmd & HBA_PxCMD_CR);
        reg->pxcmd |= HBA_PxCMD_FRE;
        reg->pxcmd |= HBA_PxCMD_ST;

        probe_disk_info(reg);
    }

}

void reset_port(struct ahci_port_registers* port_reg)
{
    // SATA-AHCI spec section 10.4.2
    port_reg->pxcmd &= ~HBA_PxCMD_ST;
    port_reg->pxcmd &= ~HBA_PxCMD_FRE;
    #define wait_until_expire(cond, max)                                       \
    ({                                                                         \
        unsigned int __wcounter__ = (max);                                     \
        while (!(cond) && __wcounter__-- > 1)                                  \
            ;                                                                  \
        __wcounter__;                                                          \
    })
    uint32_t c = wait_until_expire(!(port_reg->pxcmd & HBA_PxCMD_CR), 500000);
    if (c) {
        return;
    }
    // 如果port未响应，则继续执行重置
    port_reg->pxsctl = (port_reg->pxsctl & ~0xf) | 1;
    io_delay(100000); // 等待至少一毫秒，差不多就行了
    port_reg->pxsctl &= ~0xf;

}

void probe_disk_info(struct ahci_port_registers* port_reg)
{
    port_reg->pxie &= ~HBA_MY_IE;

    uint16_t *data = (uint16_t *) kmalloc(512);

    struct command_table *table;

    int slot = ahci_prepare_cmd(port_reg, table);

    struct command_header *hdr = &port_reg->pxclb->command[slot];

    hdr->prdt_len = 1;
    table->entries[0] = (struct prdte){
        .data_base = vaddr_to_paddr((uint32_t) data),
        .byte_count = 512 - 1
    };

    struct sata_reg_fis* cmd_fis = (struct sata_reg_fis*)table->command_fis;

    if (port_reg->pxsig == ATA_SIGNATURE) {//硬盘
        sata_create_fis(cmd_fis, ATA_IDENTIFY_DEVICE, 0, 0);
    } else if (port_reg->pxsig == ATAPI_SIGNATURE) {//光驱
//        sata_create_fis(cmd_fis, ATA_IDENTIFY_PAKCET_DEVICE, 0, 0);
        kernel_log(ERROR, "Don't Support ATAPI");
    } else {
        kernel_log(ERROR, "can't match port_reg->pxsig");
    }

    if (ahci_try_send(port_reg, slot)<0) {
        kernel_log(ERROR, "ahci_try_send");
    }

    struct ahci_device_info *info = kmalloc(sizeof(struct ahci_device_info));
    ahci_parse_dev_info(info, data);

    kernel_log(INFO, "model:%s serial_num:%s", info->model, info->serial_num);
    kernel_log(INFO, "=======================");
    while (1);
}

int __get_free_slot(struct ahci_port_registers* port_reg)
{
    uint32_t bitmap = port_reg->pxsact | port_reg->pxci;
    int i = 0;
    for (; i <= main_ahci_dev->cmd_slots_num && (bitmap & 0x1); i++, bitmap >>= 1);
    if (i>main_ahci_dev->cmd_slots_num)
    {
        return -1;
    }
    else
    {
        return i;
    }
}


int ahci_prepare_cmd(struct ahci_port_registers* port_reg, struct command_table *table)
{
    int slot = __get_free_slot(port_reg);
    if (slot<0){
        kernel_log(ERROR, "no free slot");
    }

    struct command_header *hdr = &port_reg->pxclb->command[slot];

    table = (struct command_table *) kmalloc(sizeof(struct command_table));
    memory_set(hdr, 0, sizeof(struct command_header));

    hdr->cmd_table_base = vaddr_to_paddr((uint32_t)table);
    hdr->options = HBA_CMDH_FIS_LEN(sizeof(struct sata_reg_fis)) | HBA_CMDH_CLR_BUSY;

    return slot;
}

int ahci_try_send(struct ahci_port_registers* port, int slot)
{
    int num = 0;

    while (port->pxtfd & (HBA_PxTFD_BSY | HBA_PxTFD_DRQ));

    port->pxis = -1;

    while (num < 2) {
        port->pxci = 1 << slot;

        wait_until_expire(!(port->pxci & 1 << slot), 1000000);

        port->pxci &= ~(1 << slot); // ensure CI bit is cleared
        if ((port->pxtfd & HBA_PxTFD_ERR)) {
            num++;
        } else {
            break;
        }
    }

    port->pxis = -1;

    return num < 2;
}

void ahci_parse_dev_info(struct ahci_device_info* info, uint16_t *data)
{
    uint32_t cdb_size[] = { SCSI_CDB12, SCSI_CDB16, 0, 0 };
    info->max_lba = *((uint32_t *)(data + IDDEV_OFFMAXLBA));
    info->block_size = *((uint32_t *)(data + IDDEV_OFFLSECSIZE));
    info->cbd_size = cdb_size[(*data & 0x3)];
    info->wwn = *(uint64_t *)(data + IDDEV_OFFWWN);
    info->block_per_sec = 1 << (*(data + IDDEV_OFFLPP) & 0xf);
    info->alignment_offset = *(data + IDDEV_OFFALIGN) & 0x3fff;
    info->capabilities = *((uint32_t *)(data + IDDEV_OFFCAPABILITIES));

    if (!info->block_size) {
        info->block_size = 512;
    }

    if ((*(data + IDDEV_OFFADDSUPPORT) & 0x8) &&
        (*(data + IDDEV_OFFA48SUPPORT) & 0x400)) {
        info->max_lba = *((uint64_t *)(data + IDDEV_OFFMAXLBA_EXT));
        info->flags |= HBA_DEV_FEXTLBA;
    }

    ahci_parsestr(info->serial_num, data + IDDEV_OFFSERIALNUM, 10);
    ahci_parsestr(info->model, data + IDDEV_OFFMODELNUM, 20);

}

void ahci_parsestr(char* str, uint16_t *reg_start, int size_word)
{
    int j = 0;
    for (int i = 0; i < size_word; i++, j += 2) {
        uint16_t reg = *(reg_start + i);
        str[j] = (char)(reg >> 8);
        str[j + 1] = (char)(reg & 0xff);
    }
    str[j - 1] = '\0';
//    strrtrim(str);
}