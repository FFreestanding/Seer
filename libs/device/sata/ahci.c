#include <device/sata/ahci.h>
#include <device/pci/pci.h>
#include <paging/vmm.h>
#include <paging/pmm.h>
#include <paging/page.h>
#include <idt/interrupts.h>
#include <kernel_io/heap.h>
#include <device/sata/sata.h>
#include <device/sata/disk/ata.h>
#include <device/sata/disk/scsi.h>


#define HBA_FIS_SIZE 256
#define HBA_CLB_SIZE 1024

// #define DO_HBA_FULL_RESET

static struct ahci_hba hba;

void
__ahci_hba_isr(isr_param *param);

int
ahci_init_device(struct hba_port* port);

void
achi_register_ops(struct hba_port* port);

unsigned int
ahci_get_port_usage()
{
    return hba.ports_bmp;
}

struct hba_port*
ahci_get_port(unsigned int index)
{
    if (index >= 32) {
        return 0;
    }
    return hba.ports[index];
}

void
__hba_reset_port(hba_reg_t* port_reg)
{
    // 根据：SATA-AHCI spec section 10.4.2 描述的端口重置流程
    port_reg[HBA_RPxCMD] &= ~HBA_PxCMD_ST;
    port_reg[HBA_RPxCMD] &= ~HBA_PxCMD_FRE;
    int cnt = wait_until_expire(!(port_reg[HBA_RPxCMD] & HBA_PxCMD_CR), 500000);
    if (cnt) {
        return;
    }
    // 如果port未响应，则继续执行重置
    port_reg[HBA_RPxSCTL] = (port_reg[HBA_RPxSCTL] & ~0xf) | 1;
    io_delay(100000); //等待至少一毫秒，差不多就行了
    port_reg[HBA_RPxSCTL] &= ~0xf;
}

void
ahci_init()
{
    struct pci_device* ahci_dev = pci_get_device_by_class(AHCI_HBA_CLASS);

    if (ahci_dev==0)
    {
        kernel_log(ERROR, "AHCI: Not Found");
    }

    uintptr_t bar6, size;
    size = pci_bar_sizing(ahci_dev, &bar6, 6);

    if (0 == (bar6 && PCI_BAR_MMIO(bar6)))
    {
        kernel_log(ERROR, "AHCI: BAR#6 is not MMIO.");
    }

    pci_reg_t cmd = pci_read_cspace(ahci_dev->cspace_base, PCI_REG_STATUS_CMD);

    // 禁用传统中断（因为我们使用MSI），启用MMIO访问，允许PCI设备间访问
    cmd |= (PCI_RCMD_MM_ACCESS | PCI_RCMD_DISABLE_INTR | PCI_RCMD_BUS_MASTER);

    pci_write_cspace(ahci_dev->cspace_base, PCI_REG_STATUS_CMD, cmd);

    pci_setup_msi(ahci_dev, AHCI_HBA_IV);
    interrupt_routine_subscribe(AHCI_HBA_IV, __ahci_hba_isr);

    memory_set(&hba, 0, sizeof(hba));


    hba.base = (hba_reg_t*)vmm_map_pages(PCI_BAR_ADDR_MM(bar6), PCI_BAR_ADDR_MM(bar6),
                                         DEFAULT_PAGE_FLAGS, DEFAULT_PAGE_FLAGS, size/0x1000);
        pmm_mark_chunk_occupied(physical_memory_manager_instance_get(), PCI_BAR_ADDR_MM(bar6)>>12, size/0x1000);

#ifdef DO_HBA_FULL_RESET
    // 重置HBA
    hba.base[HBA_RGHC] |= HBA_RGHC_RESET;
    wait_until(!(hba.base[HBA_RGHC] & HBA_RGHC_RESET));
#endif

    // 启用AHCI工作模式，启用中断
    hba.base[HBA_RGHC] |= HBA_RGHC_ACHI_ENABLE;
    hba.base[HBA_RGHC] |= HBA_RGHC_INTR_ENABLE;

    // As per section 3.1.1, this is 0 based value.
    hba_reg_t cap = hba.base[HBA_RCAP];
    hba_reg_t pmap = hba.base[HBA_RPI];

    hba.ports_num = (cap & 0x1f) + 1;  // CAP.PI
    hba.cmd_slots = (cap >> 8) & 0x1f; // CAP.NCS
    hba.version = hba.base[HBA_RVER];
    hba.ports_bmp = pmap;

    /* ------ HBA端口配置 ------ */
    uintptr_t clb_pg_addr, fis_pg_addr, clb_pa, fis_pa;
    for (size_t i = 0, fisp = 0, clbp = 0; i < 32;
         i++, pmap >>= 1, fisp = (fisp + 1) % 16, clbp = (clbp + 1) % 4) {
        if (!(pmap & 0x1)) {
            continue;
        }

        struct hba_port* port =
                (struct hba_port*)kmalloc(sizeof(struct hba_port));
        hba_reg_t* port_regs =
                (hba_reg_t*)(&hba.base[HBA_RPBASE + i * HBA_RPSIZE]);

#ifndef DO_HBA_FULL_RESET
        __hba_reset_port(port_regs);
#endif

        if (!clbp) {
            // 每页最多4个命令队列
            clb_pa = (uintptr_t) pmm_alloc_page_entry();
            clb_pg_addr = (uintptr_t) vmm_map_page(clb_pa, clb_pa,
                                                   DEFAULT_PAGE_FLAGS, DEFAULT_PAGE_FLAGS);
            pmm_mark_chunk_occupied(physical_memory_manager_instance_get(),
                                    clb_pa>>12, 1);
            memory_set((uint8_t *) clb_pg_addr, 0, 0x1000);
        }
        if (!fisp) {
            // 每页最多16个FIS
            fis_pa = (uintptr_t) pmm_alloc_page_entry();

            fis_pg_addr = (uintptr_t) vmm_map_page(fis_pa, fis_pa,
                                                   DEFAULT_PAGE_FLAGS, DEFAULT_PAGE_FLAGS);
            pmm_mark_chunk_occupied(physical_memory_manager_instance_get(),
                                    fis_pa>>12, 1);
            memory_set((uint8_t *) fis_pg_addr, 0, 0x1000);
        }

        /* 重定向CLB与FIS */
        port_regs[HBA_RPxCLB] = clb_pa + clbp * HBA_CLB_SIZE;
        port_regs[HBA_RPxFB] = fis_pa + fisp * HBA_FIS_SIZE;

        *port = (struct hba_port){ .regs = port_regs,
                .ssts = port_regs[HBA_RPxSSTS],
                .cmdlst = (struct hba_cmdh *) (clb_pg_addr + clbp * HBA_CLB_SIZE),
                .fis = (void *) (fis_pg_addr + fisp * HBA_FIS_SIZE)};

        /* 初始化端口，并置于就绪状态 */
        port_regs[HBA_RPxCI] = 0;

        // 需要通过全部置位去清空这些寄存器（相当的奇怪……）
        port_regs[HBA_RPxSERR] = -1;

        port_regs[HBA_RPxIE] |= (HBA_PxINTR_D2HR);

        hba.ports[i] = port;

        if (!HBA_RPxSSTS_IF(port->ssts)) {
            continue;
        }

        wait_until(!(port_regs[HBA_RPxCMD] & HBA_PxCMD_CR));
        port_regs[HBA_RPxCMD] |= HBA_PxCMD_FRE;
        port_regs[HBA_RPxCMD] |= HBA_PxCMD_ST;

        if (!ahci_init_device(port)) {
            kernel_log(ERROR, "fail to init device");
        }
    }
}

char sata_ifs[][20] = { "Not detected",
                        "SATA I (1.5Gbps)",
                        "SATA II (3.0Gbps)",
                        "SATA III (6.0Gbps)" };

void
__ahci_hba_isr(isr_param *param)
{
    // TODO: hba interrupt
    kprintf("HBA INTR\n");
}

void
ahci_list_device()
{
    kernel_log(INFO, "Version: %h; Ports: %u; Slot: %u\n",
            hba.version,
            hba.ports_num,
            hba.cmd_slots);
    struct hba_port* port;
    for (size_t i = 0; i < 32; i++) {
        port = hba.ports[i];

        // 愚蠢的gcc似乎认为 struct hba_port* 不可能为空
        //  所以将这个非常关键的if给优化掉了。
        //  这里将指针强制转换为整数，欺骗gcc :)
        if ((uintptr_t)port == 0 || port->device == 0) {
            continue;
        }

        int device_state = HBA_RPxSSTS_IF(port->ssts);

        kernel_log(INFO, "Port %u: %s (%h)",
                i,
                &sata_ifs[device_state],
                port->device->flags);

        struct hba_device* dev_info = port->device;
        if (!device_state || !dev_info) {
            continue;
        }
        kernel_log(INFO, "capacity: %u KiB",
                (dev_info->max_lba * dev_info->block_size) >> 10);
        kernel_log(INFO, "block size: %uB", dev_info->block_size);
        kernel_log(INFO, "block/sector: %u", dev_info->block_per_sec);
        kernel_log(INFO, "alignment: %uB", dev_info->alignment_offset);
        kernel_log(INFO, "capabilities: %h", dev_info->capabilities);
        kernel_log(INFO, "model: %s", &dev_info->model);
        kernel_log(INFO, "serial: %s", &dev_info->serial_num);
    }
}

int
__get_free_slot(struct hba_port* port)
{
    hba_reg_t pxsact = port->regs[HBA_RPxSACT];
    hba_reg_t pxci = port->regs[HBA_RPxCI];
    hba_reg_t free_bmp = pxsact | pxci;
    uint32_t i = 0;
    for (; i <= hba.cmd_slots && (free_bmp & 0x1); i++, free_bmp >>= 1)
        ;
    return i | -(i > hba.cmd_slots);
}

void
sata_create_fis(struct sata_reg_fis* cmd_fis,
                uint8_t command,
                uint64_t lba,
                uint16_t sector_count)
{
    cmd_fis->head.type = SATA_REG_FIS_H2D;
    cmd_fis->head.options = SATA_REG_FIS_COMMAND;
    cmd_fis->head.status_cmd = command;
    cmd_fis->dev = 0;

    cmd_fis->lba0 = SATA_LBA_COMPONENT(lba, 0);
    cmd_fis->lba8 = SATA_LBA_COMPONENT(lba, 8);
    cmd_fis->lba16 = SATA_LBA_COMPONENT(lba, 16);
    cmd_fis->lba24 = SATA_LBA_COMPONENT(lba, 24);

    cmd_fis->lba32 = SATA_LBA_COMPONENT(lba, 32);
    cmd_fis->lba40 = SATA_LBA_COMPONENT(lba, 40);

    cmd_fis->count = sector_count;
}

int
hba_prepare_cmd(struct hba_port* port,
                struct hba_cmdt** cmdt,
                struct hba_cmdh** cmdh,
                void* buffer,
                unsigned int size)
{
    int slot = __get_free_slot(port);
    if (slot < 0)
    {
        kernel_log(ERROR, "HBA: No free slot");
    }

    if (size > 0x400000)
    {
        kernel_log(ERROR, "HBA: buffer too big");
    }

    // 构建命令头（Command Header）和命令表（Command Table）
    struct hba_cmdh* cmd_header = &port->cmdlst[slot];
    struct hba_cmdt* cmd_table = kmalloc(sizeof(struct hba_cmdt));

    memory_set(cmd_header, 0, sizeof(*cmd_header));

    // 将命令表挂到命令头上
    cmd_header->cmd_table_base = vaddr_to_paddr((uint32_t) cmd_table);
    cmd_header->options =
            HBA_CMDH_FIS_LEN(sizeof(struct sata_reg_fis)) | HBA_CMDH_CLR_BUSY;

    if (buffer) {
        cmd_header->prdt_len = 1;
        cmd_table->entries[0] =
                (struct hba_prdte){ .data_base = vaddr_to_paddr(buffer),
                        .byte_count = size - 1 };
    }

    *cmdh = cmd_header;
    *cmdt = cmd_table;

    return slot;
}

int
ahci_init_device(struct hba_port* port)
{
    /* 发送ATA命令，参考：SATA AHCI Spec Rev.1.3.1, section 5.5 */
    struct hba_cmdt* cmd_table;
    struct hba_cmdh* cmd_header;

    // 确保端口是空闲的
    wait_until(!(port->regs[HBA_RPxTFD] & (HBA_PxTFD_BSY)));

    // 预备DMA接收缓存，用于存放HBA传回的数据
    uint16_t* data_in = (uint16_t*)kmalloc(512);

    int slot = hba_prepare_cmd(port, &cmd_table, &cmd_header, data_in, 512);

    // 清空任何待响应的中断
    port->regs[HBA_RPxIS] = 0;
    port->device = kmalloc(sizeof(struct hba_device));

    // 在命令表中构建命令FIS
    struct sata_reg_fis* cmd_fis = (struct sata_reg_fis*)cmd_table->command_fis;

    // 根据设备类型使用合适的命令
    if (port->regs[HBA_RPxSIG] == HBA_DEV_SIG_ATA) {
        // ATA 一般为硬盘
        sata_create_fis(cmd_fis, ATA_IDENTIFY_DEVICE, 0, 0);
    } else {
        // ATAPI 一般为光驱，软驱，或者磁带机
        port->device->flags |= HBA_DEV_FATAPI;
        sata_create_fis(cmd_fis, ATA_IDENTIFY_PAKCET_DEVICE, 0, 0);
    }

    // PxCI寄存器置位，告诉HBA这儿有个数据需要发送到SATA端口
    port->regs[HBA_RPxCI] = (1 << slot);

    wait_until(!(port->regs[HBA_RPxCI] & (1 << slot)));

    if ((port->regs[HBA_RPxTFD] & HBA_PxTFD_ERR)) {
        // 有错误
        sata_read_error(port);
        goto fail;
    }

    /*
        等待数据到达内存
        解析IDENTIFY DEVICE传回来的数据。
          参考：
            * ATA/ATAPI Command Set - 3 (ACS-3), Section 7.12.7
    */
    ahci_parse_dev_info(port->device, data_in);

    if (!(port->device->flags & HBA_DEV_FATAPI)) {
        goto done;
    }

    /*
        注意：ATAPI设备是无法通过IDENTIFY PACKET DEVICE 获取容量信息的。
        我们需要使用SCSI命令的READ_CAPACITY(16)进行获取。
        步骤如下：
            1. 因为ATAPI走的是SCSI，而AHCI对此专门进行了SATA的封装，
               也就是通过SATA的PACKET命令对SCSI命令进行封装。所以我们
               首先需要构建一个PACKET命令的FIS
            2. 接着，在ACMD中构建命令READ_CAPACITY的CDB - 一种SCSI命令的封装
            3. 然后把cmd_header->options的A位置位，表示这是一个送往ATAPI的命令。
                一点细节：
                    1. HBA往底层SATA控制器发送PACKET FIS
                    2. SATA控制器回复PIO Setup FIS
                    3. HBA读入ACMD中的CDB，打包成Data FIS进行答复
                    4. SATA控制器解包，拿到CDB，通过SCSI协议转发往ATAPI设备。
                    5. ATAPI设备回复Return Parameter，SATA通过DMA Setup FIS
                       发起DMA请求，HBA介入，将Return Parameter写入我们在PRDT
                       里设置的data_in位置。
            4. 最后照常等待HBA把结果写入data_in，然后直接解析就好了。
          参考：
            * ATA/ATAPI Command Set - 3 (ACS-3), Section 7.18
            * SATA AHCI HBA Spec, Section 5.3.7
            * SCSI Command Reference Manual, Section 3.26
    */
    struct scsi_cdb16* cdb16 = (struct scsi_cdb16*)cmd_table->atapi_cmd;

    sata_create_fis(cmd_fis, ATA_PACKET, 512 << 8, 0);
    scsi_create_packet16(cdb16, SCSI_READ_CAPACITY_16, 0, 512);

    cdb16->misc1 = 0x10; // service action
    cmd_header->transferred_size = 0;
    cmd_header->options |= HBA_CMDH_ATAPI;

    port->regs[HBA_RPxCI] = (1 << slot);
    wait_until(!(port->regs[HBA_RPxCI] & (1 << slot)));

    if ((port->regs[HBA_RPxTFD] & HBA_PxTFD_ERR)) {
        // 有错误
        sata_read_error(port);
        goto fail;
    }

    scsi_parse_capacity(port->device, (uint32_t*)data_in);

    done:
    achi_register_ops(port);

    kfree(data_in);
    kfree(cmd_table);

    return 1;

    fail:
    kfree(data_in);
    kfree(cmd_table);

    return 0;
}

int
ahci_identify_device(struct hba_port* port)
{
    // 用于重新识别设备（比如在热插拔的情况下）
    kfree(port->device);
    return ahci_init_device(port);
}

void
achi_register_ops(struct hba_port* port)
{
    port->device->ops.identify = ahci_identify_device;
    if (!(port->device->flags & HBA_DEV_FATAPI)) {
        port->device->ops.read_buffer = sata_read_buffer;
        port->device->ops.write_buffer = sata_write_buffer;
    } else {
        port->device->ops.read_buffer = scsi_read_buffer;
        port->device->ops.write_buffer = scsi_write_buffer;
    }
}

void
ahci_parse_dev_info(struct hba_device* dev_info, uint16_t* data)
{
    dev_info->max_lba = *((uint32_t*)(data + IDDEV_OFFMAXLBA));
    dev_info->block_size = *((uint32_t*)(data + IDDEV_OFFLSECSIZE));
    dev_info->cbd_size = (*data & 0x3) ? 16 : 12;
    dev_info->wwn = *(uint64_t*)(data + IDDEV_OFFWWN);
    dev_info->block_per_sec = 1 << (*(data + IDDEV_OFFLPP) & 0xf);
    dev_info->alignment_offset = *(data + IDDEV_OFFALIGN) & 0x3fff;
    dev_info->capabilities = *((uint32_t*)(data + IDDEV_OFFCAPABILITIES));

    if (!dev_info->block_size) {
        dev_info->block_size = 512;
    }

    if ((*(data + IDDEV_OFFADDSUPPORT) & 0x8)) {
        dev_info->max_lba = *((uint64_t*)(data + IDDEV_OFFMAXLBA_EXT));
        dev_info->flags |= HBA_DEV_FEXTLBA;
    }

    ahci_parsestr(&dev_info->serial_num, data + IDDEV_OFFSERIALNUM, 10);
    ahci_parsestr(&dev_info->model, data + IDDEV_OFFMODELNUM, 20);
}

void
ahci_parsestr(char* str, uint16_t* reg_start, int size_word)
{
    int j = 0;
    for (int i = 0; i < size_word; i++, j += 2) {
        uint16_t reg = *(reg_start + i);
        str[j] = (char)(reg >> 8);
        str[j + 1] = (char)(reg & 0xff);
    }
    str[j - 1] = '\0';
}

/*
static struct ahci_device_context ahci_dev_ctx;


struct ahci_device_context *
get_ahci_device_context()
{
    return &ahci_dev_ctx;
}

void
ahci_isr(isr_param* p)
{
    kernel_log(INFO, "AHCI ISR");
}




void ahci_device_init()
{
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

    uint32_t start = pci_read_bar6(&pos->address);

    if (0==(start && PCI_BAR_MMIO(start)))
    {
        kernel_log(ERROR, "BAR6 is not MMIO");
    }
    uint32_t size = ahci_sizing_addr_size(pos, start);
    kernel_log(INFO, "need to map %h", size);
    size = size / 0x1000;

    uint16_t cmd = pci_read_command((uint32_t *)&pos->address);
    // 使用MSI 启用MMIO访问
    cmd |= (PCI_RCMD_MM_ACCESS | PCI_RCMD_DISABLE_INTR | PCI_RCMD_BUS_MASTER);
    pci_write_command(&pos->address, cmd);
    __pci_set_up_msi(pos, AHCI_IV);

    interrupt_routine_subscribe(AHCI_IV, ahci_isr);


    kernel_log(INFO, "need to map %h", size);
    struct ahci_registers *ahci_regs = vmm_map_pages(start, start,
                                                    DEFAULT_PAGE_FLAGS, DEFAULT_PAGE_FLAGS, size+1);

    if (ahci_regs==NULL_POINTER) {
        kernel_log(ERROR, "Failed to map AHCI Register Space | Size:%h", size);
    }

    ahci_dev_ctx.device.ahci_regs = ahci_regs;

    ahci_regs->ghc |= HBA_RGHC_RESET;
#define wait_until(cond)                                                       \
    while (!(cond))                                                            \
        ;
    wait_until(!(ahci_regs->ghc & HBA_RGHC_RESET));

    // Enable AHCI
    // AHCI Enable (AE): When set, indicates that
    // communication to the HBA shall be via AHCI mechanisms.
    ahci_regs->ghc |= HBA_RGHC_ACHI_ENABLE;
    // Interrupt Enable (IE): This global bit enables interrupts from the HBA.
    ahci_regs->ghc |= HBA_RGHC_INTR_ENABLE;

    uint32_t cap = ahci_regs->cap;

    ahci_dev_ctx.device.port_num = (cap&0x1f)+1; // CAP.NP
    ahci_dev_ctx.device.cmd_slots_num = (cap>>8)&0x1f; // CAP.NCS
    ahci_dev_ctx.device.version = ahci_dev_ctx.device.ahci_regs->vs;
    ahci_dev_ctx.device.port_bitmap = ahci_regs->pi;

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
    uint32_t bitmap = ahci_dev_ctx.device.port_bitmap;
    uint32_t clb_pg_addr = 0, fis_pg_addr = 0;
    uint32_t clb_pa = 0, fis_pa = 0;

    // 对于每一个端口
    for (size_t i = 0, fisp = 0, clbp = 0; i < 32;
         i++, bitmap >>= 1, fisp = (fisp + 1) % 16, clbp = (clbp + 1) % 4) {
        if (!(bitmap & 0x1)) {
            continue;
        }
        kernel_log(INFO, "port %u", i);

        // 获得每一个端口
        struct ahci_port_registers *reg = &ahci_dev_ctx.device.ahci_regs->port[i];

        // 3.1 进行端口重置
        reset_port(reg);

        // 3.2 分配操作空间，设置 PxCLB 和 PxFB
        if (!clbp) {
            // 每页最多4个命令队列
            clb_pa = (uint32_t) pmm_alloc_page_entry();
            clb_pg_addr = (uint32_t) vmm_map_page(clb_pa, clb_pa,
                                                          DEFAULT_PAGE_FLAGS,
                                                          DEFAULT_PAGE_FLAGS);
            pmm_mark_chunk_occupied(physical_memory_manager_instance_get(), clb_pa>>12, 1);
            memory_set_fast((void*)clb_pg_addr, 0, 0x1000/4);
        }
        if (!fisp) {
            // 每页最多16个FIS
            fis_pa = (uint32_t) pmm_alloc_page_entry();
            fis_pg_addr = (uint32_t) vmm_map_page(fis_pa,fis_pa,
                                                          DEFAULT_PAGE_FLAGS,
                                                          DEFAULT_PAGE_FLAGS);
            pmm_mark_chunk_occupied(physical_memory_manager_instance_get(), fis_pa>>12, 1);
            memory_set_fast((void*)fis_pg_addr, 0, 0x1000/4);
        }

        */
/* 重定向CLB与FIS（设置寄存器） *//*

        reg->pxclb = (struct command_header *) (clb_pa + clbp * HBA_CLB_SIZE);
        reg->pxfb = fis_pa + fisp * HBA_FIS_SIZE;

        */
/* 保存虚拟地址到context中 *//*

        ahci_dev_ctx.dev_info.port_cmd_list[i] = (struct command_header *) (clb_pg_addr + clbp * HBA_CLB_SIZE);

        */
/* 初始化端口，并置于就绪状态 *//*

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
    io_delay(200000); // 等待至少一毫秒，差不多就行了
    port_reg->pxsctl &= ~0xf;
}

void probe_disk_info(struct ahci_port_registers* port_reg)
{
    port_reg->pxie &= ~HBA_MY_IE;

    uint16_t *data = (uint16_t *) vmm_map_page(0x600000, 0x600000, DEFAULT_PAGE_FLAGS, DEFAULT_PAGE_FLAGS);
    memory_set(data, 0, 0x1000);

    struct command_table *table;

    int slot = ahci_prepare_cmd(port_reg, table);

    // 清空待响应的中断
//    port_reg->pxis = 0;

    struct command_header *hdr = ahci_dev_ctx.dev_info.port_cmd_list[slot];

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
    kernel_log(INFO, "=======================");
    kernel_log(INFO, "model:%s serial_num:%s", info->model, info->serial_num);
    kernel_log(INFO, "=======================");
    while (1);
}

int __get_free_slot(struct ahci_port_registers* port_reg)
{
    uint32_t bitmap = port_reg->pxsact | port_reg->pxci;
    int i = 0;
    for (; i <= ahci_dev_ctx.device.cmd_slots_num && (bitmap & 0x1); i++, bitmap >>= 1);
    if (i>ahci_dev_ctx.device.cmd_slots_num)
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

    struct command_header *hdr = ahci_dev_ctx.dev_info.port_cmd_list[slot];
    memory_set(hdr, 0, sizeof(struct command_header));
    kernel_log(INFO, "sizeof(struct command_header):%h", sizeof(struct command_header));

    table = (struct command_table *) vmm_map_page(0x700000, 0x700000, DEFAULT_PAGE_FLAGS, DEFAULT_PAGE_FLAGS);
    memory_set(table, 0, sizeof(struct command_table));

    hdr->cmd_table_base = vaddr_to_paddr((uint32_t)table);
    hdr->options = HBA_CMDH_FIS_LEN(sizeof(struct sata_reg_fis)) | HBA_CMDH_CLR_BUSY;

    return slot;
}

int ahci_try_send(struct ahci_port_registers* port, int slot)
{
    int num = 0;

    while ((port->pxtfd & (HBA_PxTFD_BSY | HBA_PxTFD_DRQ)) != 0);

    port->pxis = -1;

    while (num < 2) {
        port->pxci = 1 << slot;

        wait_until_expire(!(port->pxci & (1 << slot)), 1000000);

        port->pxci &= ~(1 << slot); // ensure CI bit is cleared
        if ((port->pxtfd & HBA_PxTFD_ERR)) {
            kernel_log(ERROR, "ahci_try_send");
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
}*/
