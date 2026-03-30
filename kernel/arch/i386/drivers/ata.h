#ifndef ATA_H
#define ATA_H

#include <stdint.h>
#include "devices.h"

#define ATA_PRIMARY_BASE   0x1F0
#define ATA_SECONDARY_BASE 0x170

#define ATA_REG_DATA       0
#define ATA_REG_FEATURES   1
#define ATA_REG_ERROR      1
#define ATA_REG_SECCOUNT   2
#define ATA_REG_LBA_LO     3
#define ATA_REG_LBA_MID    4
#define ATA_REG_LBA_HI     5
#define ATA_REG_DEVICE     6
#define ATA_REG_COMMAND    7
#define ATA_REG_STATUS     7

#define ATA_CMD_READ_PIO          0x20
#define ATA_CMD_WRITE_PIO         0x30
#define ATA_CMD_READ_DMA          0xC8
#define ATA_CMD_WRITE_DMA         0xCA
#define ATA_CMD_CACHE_FLUSH       0xE7
#define ATA_CMD_IDENTIFY          0xEC

#define ATA_SR_BSY     0x80    // Busy
#define ATA_SR_DRDY    0x40    // Drive ready
#define ATA_SR_DF      0x20    // Drive write fault
#define ATA_SR_DSC     0x10    // Drive seek complete
#define ATA_SR_DRQ     0x08    // Data request ready
#define ATA_SR_CORR    0x04    // Corrected data
#define ATA_SR_IDX     0x02    // Index
#define ATA_SR_ERR     0x01    // Error

// Bus Master IDE (BMIDE) registers
#define BMIDE_REG_COMMAND    0x00
#define BMIDE_REG_STATUS     0x02
#define BMIDE_REG_PRDT_ADDR  0x04

#define BMIDE_CMD_START      0x01
#define BMIDE_CMD_READ       0x08  // 1 = Disk to Memory (DMA Read)

#define BMIDE_STATUS_INTERRUPT 0x04
#define BMIDE_STATUS_ERROR     0x02
#define BMIDE_STATUS_ACTIVE    0x01

typedef struct {
    uint32_t phys_addr;
    uint16_t size;
    uint16_t end_of_table;
} __attribute__((packed)) prd_t;

void init_ata();

#endif // ATA_H
