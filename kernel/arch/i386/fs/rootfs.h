#ifndef ROOTFS_H
#define ROOTFS_H

#include "vfs.h"

vnode_t* rootfs_mount(Device_t* dev);

#endif // ROOTFS_H
