#include "buffer.h"
#include <stdio.h>
#include "kmalloc.h"

static buffer_t* buffer_list = NULL;

static buffer_t* find_buffer(Device_t* dev, uint32_t lba) {
    buffer_t* curr = buffer_list;
    while (curr) {
        if (curr->dev == dev && curr->lba == lba) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

static buffer_t* create_buffer(Device_t* dev, uint32_t lba) {
    buffer_t* buf = kmalloc(sizeof(buffer_t));
    if (!buf) return NULL;

    buf->dev = dev;
    buf->lba = lba;
    buf->data = kmalloc(dev->sector_size);
    if (!buf->data) {
        kfree(buf);
        return NULL;
    }
    buf->dirty = 0;
    buf->refcount = 0;
    buf->next = buffer_list;
    buffer_list = buf;
    return buf;
}

buffer_t* buffer_get(Device_t* dev, uint32_t lba) {
    buffer_t* buf = find_buffer(dev, lba);
    if (buf) {
        buf->refcount++;
        return buf;
    }

    buf = create_buffer(dev, lba);
    if (!buf) return NULL;

    if (device_read(dev, lba, buf->data) != 0) {
        // Handle read error (simplified)
        printf("Buffer Error: Failed to read sector %u\n", lba);
    }
    buf->refcount = 1;
    return buf;
}

void buffer_release(buffer_t* buf) {
    if (buf && buf->refcount > 0) {
        buf->refcount--;
    }
}

void buffer_flush(buffer_t* buf) {
    if (buf && buf->dirty) {
        if (device_write(buf->dev, buf->lba, buf->data) == 0) {
            buf->dirty = 0;
        } else {
            printf("Buffer Error: Failed to flush sector %u\n", buf->lba);
        }
    }
}

void buffer_flush_all(Device_t* dev) {
    buffer_t* curr = buffer_list;
    while (curr) {
        if (curr->dev == dev || dev == NULL) {
            buffer_flush(curr);
        }
        curr = curr->next;
    }
}
