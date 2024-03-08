#ifndef SEER_ATA_H
#define SEER_ATA_H
#include <stdint.h>
#include <device/sata/ahci.h>
#include <device/sata/sata.h>
#define ATA_SIGNATURE 0x00000101

#define ATA_IDENTIFY_DEVICE 0xec
#define ATA_IDENTIFY_PAKCET_DEVICE 0xa1
#define ATA_PACKET 0xa0
#define ATA_READ_DMA_EXT 0x25
#define ATA_READ_DMA 0xc8
#define ATA_WRITE_DMA_EXT 0x35
#define ATA_WRITE_DMA 0xca

void
ahci_parse_dev_info(struct hba_device* dev_info, uint16_t* data);

void
ahci_parsestr(char* str, uint16_t* reg_start, int size_word);

void
scsi_parse_capacity(struct hba_device* device, uint32_t* parameter);


void sata_read_error(struct hba_port* port);

#endif //SEER_ATA_H
