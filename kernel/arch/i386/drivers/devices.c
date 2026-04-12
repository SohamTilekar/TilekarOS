#include "devices.h"
#include <string.h>

static device_t* device_list_head = NULL;
static device_t* device_list_tail = NULL;

int device_register(device_t* dev) {
    if (!dev) return -1;

    dev->next = NULL;

    // Append to tail of the list to act as FIFO
    if (!device_list_head)
        // First element in the list
        device_list_head = dev;
    else
        device_list_tail->next = dev;
    device_list_tail = dev;

    return 0;
}

device_t* device_get(const char* name) {
    device_t* curr = device_list_head;
    while (curr) {
        if (strcmp(curr->name, name) == 0) return curr;
        curr = curr->next;
    }
    return NULL;
}

device_t* device_get_next(device_t* current) {
    if (!current) return device_list_head;
    return current->next;
}

int device_read(device_t* dev, uint32_t lba, uint8_t* buffer) {
    if (!dev || dev->type != DEVICE_TYPE_BLOCK || !dev->read_sector) return -1;
    if (lba >= dev->total_sectors) return -2;
    return dev->read_sector(dev, lba, buffer);
}

int device_write(device_t* dev, uint32_t lba, const uint8_t* buffer) {
    if (!dev || dev->type != DEVICE_TYPE_BLOCK || !dev->write_sector) return -1;
    if (lba >= dev->total_sectors) return -2;
    return dev->write_sector(dev, lba, buffer);
}
