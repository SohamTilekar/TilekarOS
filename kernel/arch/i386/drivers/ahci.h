#ifndef AHCI_H
#define AHCI_H

#include <stdint.h>
#include <stdbool.h>
#include "pci.h"
#include "devices.h"

#define AHCI_VENDOR_ID 0x8086
#define AHCI_DEVICE_ID 0x2829

#define HBA_PORT_IPM_ACTIVE  1
#define HBA_PORT_DET_PRESENT 3

#define AHCI_PCI_CLASS     0x01
#define AHCI_PCI_SUBCLASS  0x06
#define AHCI_PCI_PROGIF    0x01

#define SATA_SIG_ATA    0x00000101
#define SATA_SIG_ATAPI  0xEB140101
#define SATA_SIG_SEMB   0xC33C0101
#define SATA_SIG_PM     0x96690101

#define HBA_GHC_AE      (1U << 31)

/* FIS Types */
typedef enum {
    FIS_TYPE_REG_H2D   = 0x27,
    FIS_TYPE_REG_D2H   = 0x34,
    FIS_TYPE_DMA_ACT   = 0x39,
    FIS_TYPE_DMA_SETUP = 0x41,
    FIS_TYPE_DATA      = 0x46,
    FIS_TYPE_BIST      = 0x58,
    FIS_TYPE_PIO_SETUP = 0x5F,
    FIS_TYPE_DEV_BITS  = 0xA1,
} FIS_TYPE;

/* Host to Device Register FIS */
typedef struct {
    uint8_t  fis_type;
    uint8_t  pmport:4;
    uint8_t  rsv0:3;
    uint8_t  c:1;
    uint8_t  command;
    uint8_t  featurel;
    uint8_t  lba0;
    uint8_t  lba1;
    uint8_t  lba2;
    uint8_t  device;
    uint8_t  lba3;
    uint8_t  lba4;
    uint8_t  lba5;
    uint8_t  featureh;
    uint8_t  countl;
    uint8_t  counth;
    uint8_t  icc;
    uint8_t  control;
    uint8_t  rsv1[4];
} fis_reg_h2d_t;

/* Physical Region Descriptor Table Entry */
typedef struct {
    uint32_t dba;
    uint32_t dbau;
    uint32_t rsv0;
    uint32_t dbc:22;
    uint32_t rsv1:9;
    uint32_t i:1;
} prdt_entry_t;

/* Command Header */
typedef struct {
    uint8_t  cfl:5;
    uint8_t  a:1;
    uint8_t  w:1;
    uint8_t  p:1;
    uint8_t  r:1;
    uint8_t  b:1;
    uint8_t  c:1;
    uint8_t  rsv0:1;
    uint8_t  pmp:4;
    uint16_t prdtl;
    volatile uint32_t prdbc;
    uint32_t ctba;
    uint32_t ctbau;
    uint32_t rsv1[4];
} hba_cmd_header_t;

/* Command Table */
typedef struct {
    uint8_t  cfis[64];
    uint8_t  acmd[16];
    uint8_t  rsv[48];
    prdt_entry_t prdt_entry[1];
} hba_cmd_tbl_t;

/*
 * FIX: All HBA register structs must be volatile.
 * These are memory-mapped hardware registers; the compiler must not cache
 * or reorder reads/writes to them. Without volatile, polling loops like
 *   while (port->tfd & 0x80) {}
 * may be optimized into infinite loops or removed entirely.
 */

/* HBA Port Registers */
typedef struct {
    volatile uint32_t clb;
    volatile uint32_t clbu;
    volatile uint32_t fb;
    volatile uint32_t fbu;
    volatile uint32_t is;
    volatile uint32_t ie;
    volatile uint32_t cmd;
    volatile uint32_t rsv0;
    volatile uint32_t tfd;
    volatile uint32_t sig;
    volatile uint32_t ssts;
    volatile uint32_t sctl;
    volatile uint32_t serr;
    volatile uint32_t sact;
    volatile uint32_t ci;
    volatile uint32_t sntf;
    volatile uint32_t fbs;
    volatile uint32_t rsv1[11];
    volatile uint32_t vendor[4];
} hba_port_t;

/* HBA Memory Registers */
typedef struct {
    volatile uint32_t cap;
    volatile uint32_t ghc;
    volatile uint32_t is;
    volatile uint32_t pi;
    volatile uint32_t vs;
    volatile uint32_t ccc_ctl;
    volatile uint32_t ccc_pts;
    volatile uint32_t em_loc;
    volatile uint32_t em_ctl;
    volatile uint32_t cap2;
    volatile uint32_t bohc;
    uint8_t  rsv[0xA0-0x2C];
    uint8_t  vendor[0x100-0xA0];
    hba_port_t ports[1];
} hba_mem_t;

typedef struct {
    hba_mem_t* hba_mem;
    int port_count;
} ahci_controller_t;

typedef struct {
    hba_port_t* port;
    int port_number;
    uint32_t sector_count;
    char name[16];
    void* clb_virt;
    void* fb_virt;
    void* ctba_virt[32];
} ahci_device_t;

void init_ahci();

#endif
