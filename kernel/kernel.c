#include "ata.h"
#include "pci.h"
#include "stdio.h"
#include <kernel/tty.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "kmalloc.h"
#include "task.h"
#include "devices.h"
#include "ramdisk.h"
#include "fat.h"
#include "vfs.h"
#include "string.h"

// Global to persist across tasks
static device_t* primary_storage = NULL;

void init_storage() {
    printf("Probing storage devices...\n");
    device_t* dev = device_get_next(NULL);
    bool root_mounted = (primary_storage != NULL);

    while (dev) {
        if (dev->type == DEVICE_TYPE_BLOCK) {
            if (!root_mounted) {
                // First block device found becomes root
                fat_format(dev, "ROOT");
                if (vfs_mount("/", dev, fat_mount)) {
                    printf("  Mounted %s as /\n", dev->name);
                    primary_storage = dev;
                    root_mounted = true;

                    // Setup initial files on root
                    fat_filesystem_t fs;
                    fat_init(&fs, dev);
                    fat_mkdir(&fs, "/BIN");
                    fat_mkdir(&fs, "/MNT");
                    const char* hello = "Hello from Dynamic Root!";
                    fat_create_file(&fs, "/BIN/HELLO.TXT", (const uint8_t*)hello, strlen(hello));
                }
            } else if (dev != primary_storage) {
                // Check if already in /MNT
                char mount_path[64];
                sprintf(mount_path, "/MNT/%s", dev->name);

                // If it's a new device, mount it
                if (vfs_mkdir(mount_path) == 0) {
                    fat_format(dev, dev->name);
                    if (vfs_mount(mount_path, dev, fat_mount)) {
                        printf("  Mounted %s as %s\n", dev->name, mount_path);

                        fat_filesystem_t fs_ext;
                        fat_init(&fs_ext, dev);
                        char msg[64];
                        sprintf(msg, "Storage device %s ready.", dev->name);
                        fat_create_file(&fs_ext, "/README.TXT", (const uint8_t*)msg, strlen(msg));
                    }
                }
            }
        }
        dev = device_get_next(dev);
    }

    if (!root_mounted) {
        printf("CRITICAL: No bootable storage found!\n");
    }
}

void device_rescan() {
    printf("\n--- System Hardware Rescan ---\n");
    pci_init();
    init_ata();
    init_storage();
}

extern char _start_user_task;
extern char _end_user_task;

void kernel_main(uint32_t magic, void* boot_info) {
  (void)magic; (void)boot_info;
  printf("Hello World!\nPrint On TilekarOS by Soham Tilekar\n");

  init_storage();

  // printf("\n--- Multitasking Stress Test ---\n");

  // task_create_user(&_start_user_task, &_end_user_task);

  printf("Main task (TID 0) entering infinite yield loop.\n");
  while (true) {
      task_yield(NULL);
  }
}
