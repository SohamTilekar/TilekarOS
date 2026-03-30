#ifndef DEVICES_H
#define DEVICES_H

#include <stdint.h>
#include <stddef.h>

typedef enum {
    DEVICE_TYPE_BLOCK,
    DEVICE_TYPE_CHAR,
    DEVICE_TYPE_NETWORK
} device_type_t;

typedef struct device device_t;

struct device {
    char name[32];
    device_type_t type;
    uint32_t sector_size;   // Relevant for block devices
    uint32_t total_sectors; // Relevant for block devices
    void* private_data;

    int (*read_sector)(device_t* dev, uint32_t lba, uint8_t* buffer);
    int (*write_sector)(device_t* dev, uint32_t lba, const uint8_t* buffer);
    
    // For character devices
    int (*read)(device_t* dev, void* buffer, uint32_t size);
    int (*write)(device_t* dev, const void* buffer, uint32_t size);

    struct device* next;
};

// Global device management
int device_register(device_t* dev);
device_t* device_get(const char* name);
device_t* device_get_next(device_t* current);

// Generic wrapper functions
int device_read(device_t* dev, uint32_t lba, uint8_t* buffer);
int device_write(device_t* dev, uint32_t lba, const uint8_t* buffer);

#endif // DEVICES_H
