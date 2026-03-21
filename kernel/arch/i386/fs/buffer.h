#ifndef BUFFER_H
#define BUFFER_H

#include <stdint.h>
#include <stddef.h>
#include "devices.h"

typedef struct buffer {
    device_t* dev;
    uint32_t lba;

    uint8_t* data;      // sector data (usually 512 bytes)
    int dirty;          // needs write-back
    int refcount;

    struct buffer* next;
} buffer_t;

// Core Buffer Cache API
buffer_t* buffer_get(device_t* dev, uint32_t lba);
void buffer_release(buffer_t* buf);
void buffer_flush(buffer_t* buf);
void buffer_flush_all(device_t* dev);

#endif // BUFFER_H
