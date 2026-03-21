#include "ramdisk.h"
#include <string.h>
#include "kmalloc.h"

static int ramdisk_read_sector(device_t* dev, uint32_t lba, uint8_t* buffer) {
    ramdisk_private_t* priv = (ramdisk_private_t*)dev->private_data;
    uint32_t offset = lba * dev->sector_size;
    memcpy(buffer, priv->buffer + offset, dev->sector_size);
    return 0;
}

static int ramdisk_write_sector(device_t* dev, uint32_t lba, const uint8_t* buffer) {
    ramdisk_private_t* priv = (ramdisk_private_t*)dev->private_data;
    uint32_t offset = lba * dev->sector_size;
    memcpy(priv->buffer + offset, buffer, dev->sector_size);
    return 0;
}

device_t* ramdisk_create(const char* name, uint8_t* buffer, uint32_t size) {
    device_t* dev = kmalloc(sizeof(device_t));
    ramdisk_private_t* priv = kmalloc(sizeof(ramdisk_private_t));
    
    strcpy(dev->name, name);
    dev->type = DEVICE_TYPE_BLOCK;
    dev->sector_size = 512;
    dev->total_sectors = size / 512;
    dev->private_data = priv;
    dev->read_sector = ramdisk_read_sector;
    dev->write_sector = ramdisk_write_sector;
    dev->read = NULL;
    dev->write = NULL;

    priv->buffer = buffer;
    priv->size = size;

    device_register(dev);
    return dev;
}
