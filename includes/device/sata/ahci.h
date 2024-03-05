#ifndef SEER_AHCI_H
#define SEER_AHCI_H
#include <stdint.h>

static inline void *get_achi_reg_base_addr();

void ahci_device_init();

uint32_t ahci_sizing_addr_size();



#endif //SEER_AHCI_H
