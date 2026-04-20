#include "ata.h"
#include "utils.h"
#include "stdio.h"
#include <string.h>
#include "kmalloc.h"
#include "pci.h"
#include "memory.h"
#include "idt.h"

typedef struct {
    uint16_t base_port;
    uint16_t ctrl_port;
    uint16_t bmide_port;
    uint8_t slave;
    Prd_t* prdt;         // PRD Table (virtual)
    uint32_t prdt_phys;  // PRD Table (physical)
    void* dma_buffer;    // DMA Buffer (virtual)
    uint32_t dma_phys;   // DMA Buffer (physical)
    int channel;         // 0 for Primary, 1 for Secondary
} AtaPrivate_t;

static volatile uint8_t ata_irq_fired[2] = {0, 0};

static void ata_irq_handler_primary(InterruptReg_t* r) {
    (void)r;
    ata_irq_fired[0] = 1;
    // Reading status clears interrupt bit on some controllers
    // but we should read it from the port anyway
}

static void ata_irq_handler_secondary(InterruptReg_t* r) {
    (void)r;
    ata_irq_fired[1] = 1;
}

static void ata_wait_irq(int channel) {
    while (!ata_irq_fired[channel]) {
        // In a real OS we would yield here
        asm volatile("pause");
    }
    ata_irq_fired[channel] = 0;
}

static void ata_wait_bsy(uint16_t base) {
    while (in_port_b(base + ATA_REG_STATUS) & ATA_SR_BSY);
}

static void ata_wait_drq(uint16_t base) {
    while (!(in_port_b(base + ATA_REG_STATUS) & ATA_SR_DRQ));
}

static int ata_dma_transfer(Device_t* dev, uint32_t lba, void* buffer, int is_write) {
    AtaPrivate_t* priv = (AtaPrivate_t*)dev->private_data;
    uint16_t base = priv->base_port;
    uint16_t bmide = priv->bmide_port;

    if (is_write) {
        memcpy(priv->dma_buffer, buffer, 512);
    }

    // Prepare PRDT
    priv->prdt[0].phys_addr = priv->dma_phys;
    priv->prdt[0].size = 512;
    priv->prdt[0].end_of_table = 0x8000;

    // Stop DMA if active
    out_port_b(bmide + BMIDE_REG_COMMAND, 0);
    // Clear Interrupt and Error bits
    out_port_b(bmide + BMIDE_REG_STATUS, in_port_b(bmide + BMIDE_REG_STATUS) | BMIDE_STATUS_INTERRUPT | BMIDE_STATUS_ERROR);
    // Set PRDT address
    out_port_l(bmide + BMIDE_REG_PRDT_ADDR, priv->prdt_phys);

    // Select Drive
    out_port_b(base + ATA_REG_DEVICE, (priv->slave ? 0xF0 : 0xE0) | ((lba >> 24) & 0x0F));
    ata_wait_bsy(base);

    // Set Read/Write bit in BM Command
    out_port_b(bmide + BMIDE_REG_COMMAND, is_write ? 0 : BMIDE_CMD_READ);

    // Set Sector Count and LBA
    out_port_b(base + ATA_REG_SECCOUNT, 1);
    out_port_b(base + ATA_REG_LBA_LO, (uint8_t)lba);
    out_port_b(base + ATA_REG_LBA_MID, (uint8_t)(lba >> 8));
    out_port_b(base + ATA_REG_LBA_HI, (uint8_t)(lba >> 16));

    // Reset IRQ flag
    ata_irq_fired[priv->channel] = 0;

    // Issue DMA command to ATA
    out_port_b(base + ATA_REG_COMMAND, is_write ? ATA_CMD_WRITE_DMA : ATA_CMD_READ_DMA);

    // Start DMA
    out_port_b(bmide + BMIDE_REG_COMMAND, (is_write ? 0 : BMIDE_CMD_READ) | BMIDE_CMD_START);

    // Wait for IRQ
    ata_wait_irq(priv->channel);

    // Stop DMA
    out_port_b(bmide + BMIDE_REG_COMMAND, 0);

    // Check Status
    uint8_t bm_status = in_port_b(bmide + BMIDE_REG_STATUS);
    uint8_t ata_status = in_port_b(base + ATA_REG_STATUS);

    // Clear Interrupt and Error bits
    out_port_b(bmide + BMIDE_REG_STATUS, bm_status | BMIDE_STATUS_INTERRUPT | BMIDE_STATUS_ERROR);

    if (bm_status & BMIDE_STATUS_ERROR) return -1;
    if (ata_status & ATA_SR_ERR) return -2;

    if (!is_write) {
        memcpy(buffer, priv->dma_buffer, 512);
    }

    return 0;
}

int ata_read_sector(Device_t* dev, uint32_t lba, uint8_t* buffer) {
    AtaPrivate_t* priv = (AtaPrivate_t*)dev->private_data;
    if (priv->bmide_port) {
        return ata_dma_transfer(dev, lba, buffer, 0);
    }

    uint16_t base = priv->base_port;
    out_port_b(base + ATA_REG_DEVICE, (priv->slave ? 0xF0 : 0xE0) | ((lba >> 24) & 0x0F));
    out_port_b(base + ATA_REG_FEATURES, 0);
    out_port_b(base + ATA_REG_SECCOUNT, 1);
    out_port_b(base + ATA_REG_LBA_LO, (uint8_t)lba);
    out_port_b(base + ATA_REG_LBA_MID, (uint8_t)(lba >> 8));
    out_port_b(base + ATA_REG_LBA_HI, (uint8_t)(lba >> 16));
    out_port_b(base + ATA_REG_COMMAND, ATA_CMD_READ_PIO);

    ata_wait_bsy(base);
    ata_wait_drq(base);

    uint16_t* ptr = (uint16_t*)buffer;
    for (int i = 0; i < 256; i++) {
        ptr[i] = in_port_w(base + ATA_REG_DATA);
    }

    return 0;
}

int ata_write_sector(Device_t* dev, uint32_t lba, const uint8_t* buffer) {
    AtaPrivate_t* priv = (AtaPrivate_t*)dev->private_data;
    if (priv->bmide_port) {
        return ata_dma_transfer(dev, lba, (void*)buffer, 1);
    }

    uint16_t base = priv->base_port;
    out_port_b(base + ATA_REG_DEVICE, (priv->slave ? 0xF0 : 0xE0) | ((lba >> 24) & 0x0F));
    out_port_b(base + ATA_REG_FEATURES, 0);
    out_port_b(base + ATA_REG_SECCOUNT, 1);
    out_port_b(base + ATA_REG_LBA_LO, (uint8_t)lba);
    out_port_b(base + ATA_REG_LBA_MID, (uint8_t)(lba >> 8));
    out_port_b(base + ATA_REG_LBA_HI, (uint8_t)(lba >> 16));
    out_port_b(base + ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);

    ata_wait_bsy(base);
    ata_wait_drq(base);

    uint16_t* ptr = (uint16_t*)buffer;
    for (int i = 0; i < 256; i++) {
        out_port_w(base + ATA_REG_DATA, ptr[i]);
    }

    out_port_b(base + ATA_REG_COMMAND, ATA_CMD_CACHE_FLUSH);
    ata_wait_bsy(base);

    return 0;
}

static void ata_identify(uint16_t base, uint16_t bmide, uint8_t slave, int channel, int dev_idx) {
    out_port_b(base + ATA_REG_DEVICE, slave ? 0xB0 : 0xA0);
    out_port_b(base + ATA_REG_SECCOUNT, 0);
    out_port_b(base + ATA_REG_LBA_LO, 0);
    out_port_b(base + ATA_REG_LBA_MID, 0);
    out_port_b(base + ATA_REG_LBA_HI, 0);
    out_port_b(base + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);

    uint8_t status = in_port_b(base + ATA_REG_STATUS);
    if (status == 0) return;

    ata_wait_bsy(base);
    uint8_t lba_mid = in_port_b(base + ATA_REG_LBA_MID);
    uint8_t lba_hi = in_port_b(base + ATA_REG_LBA_HI);
    if (lba_mid == 0x14 && lba_hi == 0xEB) return;
    if (lba_mid == 0x3C && lba_hi == 0xC3) return;

    ata_wait_drq(base);

    uint16_t info[256];
    for (int i = 0; i < 256; i++) {
        info[i] = in_port_w(base + ATA_REG_DATA);
    }

    uint32_t sectors = *((uint32_t*)(info + 60));

    Device_t* dev = kmalloc(sizeof(Device_t));
    memset(dev, 0, sizeof(Device_t));
    sprintf(dev->name, "ata%d%c", dev_idx, (slave ? 's' : 'm'));
    dev->type = DEVICE_TYPE_BLOCK;
    dev->sector_size = 512;
    dev->total_sectors = sectors;

    AtaPrivate_t* priv = kmalloc(sizeof(AtaPrivate_t));
    memset(priv, 0, sizeof(AtaPrivate_t));
    priv->base_port = base;
    priv->bmide_port = bmide;
    priv->slave = slave;
    priv->channel = channel;

    if (bmide) {
        priv->prdt = kmalloc(4096); // Large enough to align
        priv->prdt_phys = memory_get_phys((uintptr_t)priv->prdt);
        priv->dma_buffer = kmalloc(4096);
        priv->dma_phys = memory_get_phys((uintptr_t)priv->dma_buffer);
    }

    dev->private_data = priv;
    dev->read_sector = ata_read_sector;
    dev->write_sector = ata_write_sector;

    device_register(dev);
    // printf("ATA: Detected %s, %d MB %s\n", dev->name, sectors * 512 / 1024 / 1024, bmide ? "(DMA enabled)" : "(PIO only)");
}

static int controller_count = 0;

static void ata_pci_callback(pci_device_t* pci_dev) {
    if (pci_dev->class_code == 0x01 && pci_dev->subclass == 0x01) {
        uint32_t bar0 = pci_get_bar(pci_dev, 0);
        uint32_t __attribute__((unused)) bar1 = pci_get_bar(pci_dev, 1);
        uint32_t bar2 = pci_get_bar(pci_dev, 2);
        uint32_t __attribute__((unused)) bar3 = pci_get_bar(pci_dev, 3);
        uint32_t bar4 = pci_get_bar(pci_dev, 4);

        uint16_t pri_base = (bar0 & 0xFFFFFFFC) ? (bar0 & 0xFFFFFFFC) : 0x1F0;
        uint16_t sec_base = (bar2 & 0xFFFFFFFC) ? (bar2 & 0xFFFFFFFC) : 0x170;
        uint16_t bmide_base = (bar4 & 0xFFFFFFFC);

        if (bmide_base) {
            // Enable Bus Mastering in PCI Command register
            uint32_t pci_command = pci_config_read(pci_dev->bus, pci_dev->slot, pci_dev->func, 0x04);
            pci_config_write(pci_dev->bus, pci_dev->slot, pci_dev->func, 0x04, pci_command | 0x04);
        }

        int c_idx = controller_count++;
        ata_identify(pri_base, bmide_base ? bmide_base : 0, 0, 0, c_idx * 2);
        ata_identify(pri_base, bmide_base ? bmide_base : 0, 1, 0, c_idx * 2);
        ata_identify(sec_base, bmide_base ? bmide_base + 8 : 0, 0, 1, c_idx * 2 + 1);
        ata_identify(sec_base, bmide_base ? bmide_base + 8 : 0, 1, 1, c_idx * 2 + 1);
    }
}

void init_ata() {
    // printf("Probing ATA controllers via PCI...\n");
    irq_install_handler(14, ata_irq_handler_primary);
    irq_install_handler(15, ata_irq_handler_secondary);
    pci_scan(ata_pci_callback);
}
