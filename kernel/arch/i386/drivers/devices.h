#ifndef DEVICES_H
#define DEVICES_H

#include <stdint.h>
#include <stddef.h>

typedef enum {
    DEVICE_TYPE_BLOCK,
    DEVICE_TYPE_CHAR,
    DEVICE_TYPE_NETWORK
} DeviceType_t;

typedef struct device Device_t;

struct device {
    char name[32];
    DeviceType_t type;
    uint32_t sector_size;   // Relevant for block devices
    uint32_t total_sectors; // Relevant for block devices
    void* private_data;

    int (*read_sector)(Device_t* dev, uint32_t lba, uint8_t* buffer);
    int (*write_sector)(Device_t* dev, uint32_t lba, const uint8_t* buffer);
    
    // For character devices
    int (*read)(Device_t* dev, void* buffer, uint32_t size);
    int (*write)(Device_t* dev, const void* buffer, uint32_t size);

    struct device* next;
};

// Global device management
int device_register(Device_t* dev);
Device_t* device_get(const char* name);
Device_t* device_get_next(Device_t* current);

// Generic wrapper functions
int device_read(Device_t* dev, uint32_t lba, uint8_t* buffer);
int device_write(Device_t* dev, uint32_t lba, const uint8_t* buffer);

#endif // DEVICES_H
