#ifndef SEER_SATA_H
#define SEER_SATA_H

#define SATA_REG_FIS_D2H 0x34
#define SATA_REG_FIS_H2D 0x27
#define SATA_REG_FIS_COMMAND 0x80
#define SATA_LBA_COMPONENT(lba, offset) ((((lba) >> (offset)) & 0xff))


#define IDDEV_OFFMAXLBA 60
#define IDDEV_OFFMAXLBA_EXT 230
#define IDDEV_OFFLSECSIZE 117
#define IDDEV_OFFWWN 108
#define IDDEV_OFFSERIALNUM 10
#define IDDEV_OFFMODELNUM 27
#define IDDEV_OFFADDSUPPORT 69
#define IDDEV_OFFA48SUPPORT 83
#define IDDEV_OFFALIGN 209
#define IDDEV_OFFLPP 106
#define IDDEV_OFFCAPABILITIES 49

// LBA -> SATA控制器 -> C(柱面号):H(磁头号):S(扇区号) -> 磁盘控制器
struct sata_fis_head
{
    uint8_t type;
    uint8_t options;
    uint8_t status_cmd;
    uint8_t feat_err;
} __attribute__((packed));

struct sata_reg_fis
{
    struct sata_fis_head head;

    uint8_t lba0, lba8, lba16;
    uint8_t dev;
    uint8_t lba24, lba32, lba40;
    uint8_t feature;

    uint16_t count;

    uint8_t reserved[6];
} __attribute__((packed));

static inline void
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


#endif //SEER_SATA_H
