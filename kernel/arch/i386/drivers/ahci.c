#include "ahci.h"
#include "kmalloc.h"
#include "memory.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>

#define AHCI_BASE_VADDR 0xE0000000

static int ahci_port_rebase(ahci_device_t* ahci_dev);
static void ahci_stop_cmd(hba_port_t* port);
static void ahci_start_cmd(hba_port_t* port);
static int ahci_identify(ahci_device_t* ahci_dev, uint32_t* out_sector_count);

static int ahci_read_sector(device_t* dev, uint32_t lba, uint8_t* buffer);
static int ahci_write_sector(device_t* dev, uint32_t lba, const uint8_t* buffer);

static void ahci_pci_callback(pci_device_t* pci_dev) {
    if (pci_dev->class_code != AHCI_PCI_CLASS   ||
        pci_dev->subclass   != AHCI_PCI_SUBCLASS ||
        pci_dev->prog_if    != AHCI_PCI_PROGIF) {
        return;
    }

    printf("Found AHCI Controller at %x:%x.%d\n",
           (uint32_t)pci_dev->bus,
           (uint32_t)pci_dev->slot,
           (uint32_t)pci_dev->func);

    uint32_t abar_phys = pci_get_bar(pci_dev, 5);
    if (abar_phys == 0) return;

    /* Map ABAR to virtual address (32 KB = 8 pages) */
    for (int i = 0; i < 8; i++) {
        memory_map_page(AHCI_BASE_VADDR + i * PAGE_SIZE,
                        abar_phys  + i * PAGE_SIZE,
                        PAGE_FLAG_WRITE | PAGE_FLAG_PRESENT);
    }

    hba_mem_t* hba_mem = (hba_mem_t*)AHCI_BASE_VADDR;

    /* Reset HBA with a timeout */
    hba_mem->ghc |= (1 << 0); /* HR – HBA Reset */
    int reset_timeout = 1000000;
    while ((hba_mem->ghc & (1 << 0)) && --reset_timeout);
    if (reset_timeout == 0) {
        printf("AHCI: HBA reset timed out!\n");
        return;
    }

    /* Enable AHCI, disable global interrupts */
    hba_mem->ghc |= HBA_GHC_AE;
    hba_mem->ghc &= ~(1 << 1); /* IE = 0 */

    printf("AHCI Version: %x\n", hba_mem->vs);

    uint32_t pi = hba_mem->pi;
    for (int i = 0; i < 32; i++) {
        if (!(pi & (1 << i))) continue;

        uint32_t ssts = hba_mem->ports[i].ssts;
        uint8_t  ipm  = (ssts >> 8) & 0x0F;
        uint8_t  det  =  ssts       & 0x0F;

        if (det != HBA_PORT_DET_PRESENT || ipm != HBA_PORT_IPM_ACTIVE) continue;

        ahci_device_t* ahci_dev = kcalloc(1, sizeof(ahci_device_t));
        ahci_dev->port        = &hba_mem->ports[i];
        ahci_dev->port_number = i;
        sprintf(ahci_dev->name, "ahci%d", i);

        if (ahci_port_rebase(ahci_dev) != 0) {
            printf("AHCI: port %d rebase failed\n", i);
            kfree(ahci_dev);
            continue;
        }

        /* Wait for device to post its D2H FIS (BSY clears) */
        int tfd_timeout = 2000000;
        while ((hba_mem->ports[i].tfd & 0x80) && --tfd_timeout);
        if (tfd_timeout == 0) {
            printf("AHCI: port %d TFD BSY timeout, skipping\n", i);
            ahci_stop_cmd(&hba_mem->ports[i]);
            kfree(ahci_dev);
            continue;
        }

        uint32_t sig = hba_mem->ports[i].sig;
        printf("AHCI: port %d sig=%x\n", i, sig);

        if (sig != SATA_SIG_ATA) {
            /* Not a plain SATA drive (could be ATAPI, PM, etc.) */
            printf("AHCI: port %d not a SATA drive (sig=%x), skipping\n", i, sig);
            ahci_stop_cmd(&hba_mem->ports[i]);
            kfree(ahci_dev);
            continue;
        }

        printf("SATA drive found on port %d\n", i);

        uint32_t sector_count = 0;
        if (ahci_identify(ahci_dev, &sector_count) != 0 || sector_count == 0) {
            printf("AHCI: port %d IDENTIFY failed\n", i);
            kfree(ahci_dev);
            continue;
        }
        ahci_dev->sector_count = sector_count;
        printf("AHCI port %d: %u sectors (%u MB)\n",
               i, sector_count, sector_count / 2048);

        device_t* dev = kcalloc(1, sizeof(device_t));
        strcpy(dev->name, ahci_dev->name);
        dev->type          = DEVICE_TYPE_BLOCK;
        dev->sector_size   = 512;
        dev->total_sectors = sector_count;
        dev->private_data  = ahci_dev;
        dev->read_sector   = ahci_read_sector;
        dev->write_sector  = ahci_write_sector;

        device_register(dev);
    }
}

void init_ahci() {
    pci_scan(ahci_pci_callback);
}

/* ------------------------------------------------------------------ */

static int ahci_port_rebase(ahci_device_t* ahci_dev) {
    hba_port_t* port = ahci_dev->port;
    ahci_stop_cmd(port);

    /* Command list – 1 KB aligned */
    ahci_dev->clb_virt = kmalloc_aligned(1024, 1024);
    port->clb  = memory_get_phys((uintptr_t)ahci_dev->clb_virt);
    port->clbu = 0;
    memset(ahci_dev->clb_virt, 0, 1024);

    /* FIS base – 256-byte aligned */
    ahci_dev->fb_virt = kmalloc_aligned(256, 256);
    port->fb  = memory_get_phys((uintptr_t)ahci_dev->fb_virt);
    port->fbu = 0;
    memset(ahci_dev->fb_virt, 0, 256);

    /* Command tables – one per slot, 128-byte aligned */
    hba_cmd_header_t* cmdhdr = (hba_cmd_header_t*)ahci_dev->clb_virt;
    for (int i = 0; i < 32; i++) {
        /* prdtl is set at command-issue time; leave it 0 here */
        cmdhdr[i].prdtl = 0;
        ahci_dev->ctba_virt[i] = kmalloc_aligned(256, 128);
        cmdhdr[i].ctba  = memory_get_phys((uintptr_t)ahci_dev->ctba_virt[i]);
        cmdhdr[i].ctbau = 0;
        memset(ahci_dev->ctba_virt[i], 0, 256);
    }

    port->ie   = 0;          /* Disable port interrupts */
    port->serr = 0xFFFFFFFF; /* Clear SATA errors */
    port->is   = 0xFFFFFFFF; /* Clear pending interrupt bits */

    ahci_start_cmd(port);
    return 0;
}

static void ahci_stop_cmd(hba_port_t* port) {
    port->cmd &= ~0x0001; /* Clear ST  (bit 0) */
    port->cmd &= ~0x0010; /* Clear FRE (bit 4) */

    int timeout = 1000000;
    while ((port->cmd & 0x4000 || port->cmd & 0x8000) && timeout--)
        ; /* Wait for FR (bit 14) and CR (bit 15) to clear */
}

static void ahci_start_cmd(hba_port_t* port) {
    int timeout = 1000000;
    while ((port->cmd & 0x8000) && timeout--)
        ; /* Wait for CR to clear before setting FRE / ST */
    port->cmd |= 0x0010; /* FRE */
    port->cmd |= 0x0001; /* ST  */
}

static int ahci_find_cmd_slot(hba_port_t* port) {
    uint32_t slots = port->sact | port->ci;
    for (int i = 0; i < 32; i++) {
        if ((slots & (1 << i)) == 0) return i;
    }
    return -1;
}

/* ------------------------------------------------------------------ */

/*
 * ahci_identify
 *
 * Issues an IDENTIFY DEVICE command (0xEC) and reads back the 512-byte
 * identify data block.  Words 60-61 hold the 28-bit LBA sector count;
 * words 100-103 hold the 48-bit LBA count.  We use the 48-bit value when
 * the drive supports it (bit 10 of word 83), otherwise fall back to 28-bit.
 *
 * This is the only reliable way to learn how many sectors a drive has, and
 * it also avoids the device_read/write guard   `if (lba >= total_sectors)`
 * from always firing when total_sectors is 0.
 */
static int ahci_identify(ahci_device_t* ahci_dev, uint32_t* out_sector_count) {
    hba_port_t* port = ahci_dev->port;

    /* Allocate a 512-byte DMA buffer for the IDENTIFY response */
    uint8_t* identify_buf = kmalloc_aligned(512, 2); /* word-aligned */
    if (!identify_buf) return -1;
    memset(identify_buf, 0, 512);

    port->is = 0xFFFFFFFF;
    int slot = ahci_find_cmd_slot(port);
    if (slot == -1) { kfree(identify_buf); return -1; }

    hba_cmd_header_t* cmdhdr = (hba_cmd_header_t*)ahci_dev->clb_virt + slot;
    cmdhdr->cfl   = sizeof(fis_reg_h2d_t) / sizeof(uint32_t);
    cmdhdr->w     = 0;   /* Device → host (read) */
    cmdhdr->prdtl = 1;
    cmdhdr->p     = 0;
    cmdhdr->c     = 1;

    hba_cmd_tbl_t* cmdtbl = (hba_cmd_tbl_t*)ahci_dev->ctba_virt[slot];
    memset(cmdtbl, 0, sizeof(hba_cmd_tbl_t));

    cmdtbl->prdt_entry[0].dba  = memory_get_phys((uintptr_t)identify_buf);
    cmdtbl->prdt_entry[0].dbau = 0;
    cmdtbl->prdt_entry[0].dbc  = 511; /* 512 bytes – 1 */
    cmdtbl->prdt_entry[0].i    = 0;

    fis_reg_h2d_t* fis = (fis_reg_h2d_t*)(&cmdtbl->cfis);
    memset(fis, 0, sizeof(fis_reg_h2d_t));
    fis->fis_type = FIS_TYPE_REG_H2D;
    fis->c        = 1;
    fis->command  = 0xEC; /* IDENTIFY DEVICE */
    fis->device   = 0;    /* No LBA mode bit for IDENTIFY */

    /* Wait until BSY and DRQ are clear */
    int spin = 0;
    while ((port->tfd & (0x80 | 0x08)) && spin++ < 1000000);
    if (spin >= 1000000) { kfree(identify_buf); return -1; }

    port->ci = (1 << slot);

    /* Poll for completion */
    while (1) {
        if ((port->ci & (1 << slot)) == 0) break;
        if (port->is & (1 << 30)) {
            port->is = 0xFFFFFFFF;
            kfree(identify_buf);
            return -1;
        }
    }

    port->is = 0xFFFFFFFF;

    /*
     * Parse the identify data (little-endian 16-bit words).
     * Word 83 bit 10: 48-bit LBA supported.
     * Words 100-103:  48-bit LBA total sector count.
     * Words 60-61:    28-bit LBA total sector count.
     */
    uint16_t* id = (uint16_t*)identify_buf;
    uint32_t sectors;

    if (id[83] & (1 << 10)) {
        /* 48-bit LBA.  We only keep the low 32 bits since total_sectors is
         * uint32_t.  For drives > 2 TB you'd need a uint64_t. */
        uint64_t lba48 =  (uint64_t)id[100]
                       | ((uint64_t)id[101] << 16)
                       | ((uint64_t)id[102] << 32)
                       | ((uint64_t)id[103] << 48);
        sectors = (lba48 > 0xFFFFFFFFULL) ? 0xFFFFFFFF : (uint32_t)lba48;
    } else {
        sectors = (uint32_t)id[60] | ((uint32_t)id[61] << 16);
    }

    kfree(identify_buf);
    *out_sector_count = sectors;
    return 0;
}

/* ------------------------------------------------------------------ */

static int ahci_read_sector(device_t* dev, uint32_t lba, uint8_t* buffer) {
    ahci_device_t* ahci_dev = (ahci_device_t*)dev->private_data;
    hba_port_t*    port     = ahci_dev->port;

    port->is = 0xFFFFFFFF;
    int slot = ahci_find_cmd_slot(port);
    if (slot == -1) return -1;

    hba_cmd_header_t* cmdhdr = (hba_cmd_header_t*)ahci_dev->clb_virt + slot;
    cmdhdr->cfl   = sizeof(fis_reg_h2d_t) / sizeof(uint32_t);
    cmdhdr->w     = 0;
    cmdhdr->prdtl = 1;
    cmdhdr->p     = 0;
    cmdhdr->c     = 1;

    hba_cmd_tbl_t* cmdtbl = (hba_cmd_tbl_t*)ahci_dev->ctba_virt[slot];
    memset(cmdtbl, 0, sizeof(hba_cmd_tbl_t));

    cmdtbl->prdt_entry[0].dba  = memory_get_phys((uintptr_t)buffer);
    cmdtbl->prdt_entry[0].dbau = 0;
    cmdtbl->prdt_entry[0].dbc  = 511; /* 512 – 1 */
    cmdtbl->prdt_entry[0].i    = 0;

    fis_reg_h2d_t* fis = (fis_reg_h2d_t*)(&cmdtbl->cfis);
    memset(fis, 0, sizeof(fis_reg_h2d_t));
    fis->fis_type = FIS_TYPE_REG_H2D;
    fis->c        = 1;
    fis->command  = 0x25; /* READ DMA EXT */

    fis->lba0   = (uint8_t) lba;
    fis->lba1   = (uint8_t)(lba >>  8);
    fis->lba2   = (uint8_t)(lba >> 16);
    fis->device = 1 << 6; /* LBA mode */
    fis->lba3   = (uint8_t)(lba >> 24);
    fis->lba4   = 0;
    fis->lba5   = 0;
    fis->countl = 1;
    fis->counth = 0;

    int spin = 0;
    while ((port->tfd & (0x80 | 0x08)) && spin++ < 1000000);
    if (spin >= 1000000) return -1;

    port->ci = (1 << slot);

    while (1) {
        if ((port->ci & (1 << slot)) == 0) break;
        if (port->is & (1 << 30)) {
            port->is = 0xFFFFFFFF;
            return -1;
        }
    }

    int err = (port->is & (1 << 30)) ? -1 : 0;
    port->is = 0xFFFFFFFF;
    return err;
}

static int ahci_write_sector(device_t* dev, uint32_t lba, const uint8_t* buffer) {
    ahci_device_t* ahci_dev = (ahci_device_t*)dev->private_data;
    hba_port_t*    port     = ahci_dev->port;

    port->is = 0xFFFFFFFF;
    int slot = ahci_find_cmd_slot(port);
    if (slot == -1) return -1;

    hba_cmd_header_t* cmdhdr = (hba_cmd_header_t*)ahci_dev->clb_virt + slot;
    cmdhdr->cfl   = sizeof(fis_reg_h2d_t) / sizeof(uint32_t);
    cmdhdr->w     = 1;
    cmdhdr->prdtl = 1;
    cmdhdr->p     = 0;
    cmdhdr->c     = 1;

    hba_cmd_tbl_t* cmdtbl = (hba_cmd_tbl_t*)ahci_dev->ctba_virt[slot];
    memset(cmdtbl, 0, sizeof(hba_cmd_tbl_t));

    cmdtbl->prdt_entry[0].dba  = memory_get_phys((uintptr_t)buffer);
    cmdtbl->prdt_entry[0].dbau = 0;
    cmdtbl->prdt_entry[0].dbc  = 511;
    cmdtbl->prdt_entry[0].i    = 0;

    fis_reg_h2d_t* fis = (fis_reg_h2d_t*)(&cmdtbl->cfis);
    memset(fis, 0, sizeof(fis_reg_h2d_t));
    fis->fis_type = FIS_TYPE_REG_H2D;
    fis->c        = 1;
    fis->command  = 0x35; /* WRITE DMA EXT */

    fis->lba0   = (uint8_t) lba;
    fis->lba1   = (uint8_t)(lba >>  8);
    fis->lba2   = (uint8_t)(lba >> 16);
    fis->device = 1 << 6;
    fis->lba3   = (uint8_t)(lba >> 24);
    fis->lba4   = 0;
    fis->lba5   = 0;
    fis->countl = 1;
    fis->counth = 0;

    int spin = 0;
    while ((port->tfd & (0x80 | 0x08)) && spin++ < 1000000);
    if (spin >= 1000000) return -1;

    port->ci = (1 << slot);

    while (1) {
        if ((port->ci & (1 << slot)) == 0) break;
        if (port->is & (1 << 30)) {
            port->is = 0xFFFFFFFF;
            return -1;
        }
    }

    int err = (port->is & (1 << 30)) ? -1 : 0;
    port->is = 0xFFFFFFFF;
    return err;
}
