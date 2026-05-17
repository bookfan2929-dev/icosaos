#ifndef ATA_H
#define ATA_H
#include "io.h"

#define ATA_PRIMARY_DATA         0x1F0
#define ATA_PRIMARY_ERR          0x1F1
#define ATA_PRIMARY_SECCOUNT     0x1F2
#define ATA_PRIMARY_LBA_LOW      0x1F3
#define ATA_PRIMARY_LBA_MID      0x1F4
#define ATA_PRIMARY_LBA_HIGH     0x1F5
#define ATA_PRIMARY_DRIVE        0x1F6
#define ATA_PRIMARY_STATUS       0x1F7
#define ATA_PRIMARY_COMMAND      0x1F7

// ATA Commands & Status Bits
#define ATA_CMD_READ_SECTORS     0x20
#define ATA_STATUS_BSY           0x80  // Busy bit
#define ATA_STATUS_DRQ           0x08  // Data Request bit[201~]

int ata_read_sector(uint32_t lba, uint8_t *target_buffer);

#endif
