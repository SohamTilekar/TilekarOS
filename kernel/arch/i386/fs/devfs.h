#ifndef DEVFS_H
#define DEVFS_H

#include "vfs.h"
#include "devices.h"

void devfs_init(void);
vnode_t* devfs_get_root(void);
void devfs_on_device_registered(device_t* dev);

#endif // DEVFS_H
