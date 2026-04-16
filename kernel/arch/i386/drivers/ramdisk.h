#ifndef RAMDISK_H
#define RAMDISK_H

#include <stddef.h>
#include <stdint.h>
#include "devices.h"

typedef struct {
    uint8_t* buffer;
    uint32_t size; // in bytes
} ramdisk_private_t;

Device_t* ramdisk_create(const char* name, uint8_t* buffer, uint32_t size);

#endif // RAMDISK_H
