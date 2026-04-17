#include "stdio.h"
#include <kernel/tty.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "task.h"
#include "devices.h"
#include "fat.h"
#include "vfs.h"
#include "string.h"
#include "test/test_runner.h"

// Global to persist across tasks
static Device_t* primary_storage = NULL;

void init_storage() {
    // printf("Probing storage devices...\n");

    // Scan all block devices
    Device_t* dev = device_get_next(NULL);
    while (dev) {
        if (dev->type == DEVICE_TYPE_BLOCK) {
            if (!primary_storage) {
                fat_filesystem_t fs_check;
                if (fat_init(&fs_check, dev) != 0) {
                    fat_format(dev, "ROOT");
                }
                if (vfs_mount("/", dev, fat_mount)) {
                    printf("  Mounted %s as /\n", dev->name);
                    primary_storage = dev;
                    vfs_mkdir("/bin");
                    vfs_mkdir("/mnt");
                }
            } else if (dev != primary_storage) {
                // Mount other devices in /mnt
                char mount_path[64];
                sprintf(mount_path, "/mnt/%s", dev->name);
                vfs_mkdir(mount_path);

                fat_filesystem_t fs_check;
                if (fat_init(&fs_check, dev) != 0) {
                     fat_format(dev, dev->name);
                }

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
        dev = device_get_next(dev);
    }

    if (!primary_storage) {
        printf("CRITICAL: No bootable storage found!\n");
    }
}

extern char _start_user_task;
extern char _end_user_task;

void kernel_main(uint32_t magic, void* boot_info) {
  (void)magic; (void)boot_info;
  printf("Hello World!\nPrint On TilekarOS by Soham Tilekar\n");

  init_storage();
  task_init_scheduler();
  run_all_tests(primary_storage);

  // Launch the shell
  printf("Launching Shell (/bin/sh)...\n");
  if (!task_create_elf_from_file("/bin/sh", 3)) {
      printf("CRITICAL: Failed to launch shell!\n");
  }

  printf("Main task (TID 0) entering infinite yield loop.\n");
  while (true) {
      task_yield(NULL);
  }
}
