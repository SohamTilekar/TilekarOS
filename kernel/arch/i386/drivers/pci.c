#include "pci.h"
#include "devices.h"
#include "utils.h"
#include "stdio.h"

uint32_t pci_config_read(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address;
    uint32_t lbus  = (uint32_t)bus;
    uint32_t lslot = (uint32_t)slot;
    uint32_t lfunc = (uint32_t)func;

    address = (uint32_t)((lbus << 16) | (lslot << 11) |
              (lfunc << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));

    out_port_l(PCI_CONFIG_ADDRESS, address);
    return in_port_l(PCI_CONFIG_DATA);
}

void pci_config_write(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value) {
    uint32_t address;
    uint32_t lbus  = (uint32_t)bus;
    uint32_t lslot = (uint32_t)slot;
    uint32_t lfunc = (uint32_t)func;

    address = (uint32_t)((lbus << 16) | (lslot << 11) |
              (lfunc << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));

    out_port_l(PCI_CONFIG_ADDRESS, address);
    out_port_l(PCI_CONFIG_DATA, value);
}

uint32_t pci_get_bar(pci_device_t* dev, uint8_t bar_index) {
    if (bar_index > 5) return 0;
    return pci_config_read(dev->bus, dev->slot, dev->func, 0x10 + (bar_index * 4));
}
void pci_scan(pci_callback_t callback) {
    for (int bus = 0; bus < 256; bus++) {
        for (int slot = 0; slot < 32; slot++) {
            for (int func = 0; func < 8; func++) {
                uint32_t data = pci_config_read(bus, slot, func, 0);
                uint16_t vendor_id = data & 0xFFFF;
                if (vendor_id == 0xFFFF) continue;

                uint32_t data8 = pci_config_read(bus, slot, func, 0x08);

                pci_device_t dev;
                dev.bus = bus;
                dev.slot = slot;
                dev.func = func;
                dev.vendor_id = vendor_id;
                dev.device_id = (data >> 16) & 0xFFFF;
                dev.class_code = (data8 >> 24) & 0xFF;
                dev.subclass = (data8 >> 16) & 0xFF;
                dev.prog_if = (data8 >> 8) & 0xFF;

                callback(&dev);

                // If not multi-function, skip other functions
                if (func == 0) {
                    uint32_t header = pci_config_read(bus, slot, 0, 0x0C);
                    if (!(header & 0x800000)) break;
                }
            }
        }
    }
}

static void pci_debug_callback(pci_device_t* dev) {
    printf("PCI: %x:%x.%d - Vendor: %x Device: %x Class: %x Sub: %x\n",
           (uint32_t)dev->bus, (uint32_t)dev->slot, (uint32_t)dev->func,
           (uint32_t)dev->vendor_id, (uint32_t)dev->device_id,
           (uint32_t)dev->class_code, (uint32_t)dev->subclass);
}

void pci_init() {
    printf("Scanning PCI bus...\n");
    pci_scan(pci_debug_callback);
}
