#include <stdint.h>
#include "headers/ata.h"

static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ volatile ("inw %w1, %w0" : "=a"(ret) : "Nd"(port) : "memory");
    return ret;
}

int ata_read_sector(uint32_t lba, uint8_t *target_buffer) {
    // 1. Send the drive selection and top 4 bits of the LBA
    // 0xE0 selects the Primary Master drive and enables LBA mode
    outb(ATA_PRIMARY_DRIVE, 0xE0 | ((lba >> 24) & 0x0F));
    
    // 2. Send the sector count (we want to read exactly 1 sector)
    outb(ATA_PRIMARY_SECCOUNT, 1);
    
    // 3. Send the remaining bits of the LBA address
    outb(ATA_PRIMARY_LBA_LOW,  (uint8_t)lba);
    outb(ATA_PRIMARY_LBA_MID,  (uint8_t)(lba >> 8));
    outb(ATA_PRIMARY_LBA_HIGH, (uint8_t)(lba >> 16));
    
    // 4. Send the standard READ command
    outb(ATA_PRIMARY_COMMAND, ATA_CMD_READ_SECTORS);
    
    // 5. Poll the status register until the drive is ready
    // Wait for the Busy bit (BSY) to clear
    while (inb(ATA_PRIMARY_STATUS) & ATA_STATUS_BSY);
    
    // Wait for the Data Request bit (DRQ) to set, indicating data is ready
    while (!(inb(ATA_PRIMARY_STATUS) & ATA_STATUS_DRQ));
    
    // 6. Read 256 words (512 bytes) from the controller's data port into RAM
    uint16_t *buf16 = (uint16_t *)target_buffer;
    for (int i = 0; i < 256; i++) {
        buf16[i] = inw(ATA_PRIMARY_DATA);
    }
    
    return 1; // Success
}
