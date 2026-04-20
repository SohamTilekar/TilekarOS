#include "devices.h"
#include "fat.h"
#include "stdio.h"
#include "string.h"
#include "task.h"
#include "test/test_runner.h"
#include "vfs.h"
#include <kernel/tty.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Global to persist across tasks
static Device_t *primary_storage = NULL;

void init_storage() {
  // printf("Probing storage devices...\n");

  // Scan all block devices
  Device_t *dev = device_get_next(NULL);
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
          fat_create_file(&fs_ext, "/README.TXT", (const uint8_t *)msg,
                          strlen(msg));
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

void kernel_main(uint32_t magic, void *boot_info) {
  (void)magic;
  (void)boot_info;
  task_init_scheduler();
  printf("Hello World!\nPrint On TilekarOS by Soham Tilekar\n");

  init_storage();

  bool run_tests = false;
#if ENABLE_TEST == 1
  run_tests = true;
  printf("Running tests (ENABLE_TEST flag is set).\n");
#else
  // printf("Run built-in tests? (y/N) ");
  // char choice = 0;
  // keyboard_clear_buffer();
  // uint32_t verify_start = get_ticks();
  // while (choice == 0 && get_ticks() < verify_start + 4000) { // 4s timeout
  //   choice = keyboard_getchar();
  // }
  // printf("\n");
  // run_tests = (choice == 'y' || choice == 'Y'); // default to no on timeout
#endif

  if (run_tests) {
    run_all_tests(primary_storage);
  } else {
    printf("Skipping tests.\n");
  }
  printf("Launching Init (/bin/init)...\n");
  task_t* init_task = task_create_elf_from_file("/bin/init", 3);
  if (!init_task) {
    printf("CRITICAL: Failed to launch init!\n");
  }
  task_exit(); // exiting Main as its work as tmp process for switching is done
}
