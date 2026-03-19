#include "stdio.h"
#include <kernel/tty.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "arch/i386/kmalloc.h"
#include "arch/i386/task.h"
#include "arch/i386/devices.h"
#include "arch/i386/ramdisk.h"
#include "arch/i386/fat.h"
#include "arch/i386/vfs.h"
#include "string.h"

// Global to persist across tasks
static uint8_t* rd_buffer = NULL;
static device_t* ram0 = NULL;

void init_storage() {
    uint32_t rd_size = 1440 * 1024;
    rd_buffer = kmalloc(rd_size);
    memset(rd_buffer, 0, rd_size);

    ram0 = ramdisk_create("ram0", rd_buffer, rd_size);
    fat_format(ram0, "TILEKAROS");

    if (vfs_mount("/", ram0, fat_mount)) {
        printf("Mounted FAT filesystem to /\n");
    }

    // Setup initial files
    fat_filesystem_t fs;
    fat_init(&fs, ram0);
    fat_mkdir(&fs, "/BIN");
    const char* hello = "Hello from a VFS-accessed file!";
    fat_create_file(&fs, "/BIN/HELLO.TXT", (const uint8_t*)hello, strlen(hello));
}

extern char _start_user_task;
extern char _end_user_task;

void kernel_main(uint32_t magic, void* boot_info) {
  (void)magic; (void)boot_info;
  printf("Hello World!\nPrint On TilekarOS by Soham Tilekar\n");

  init_storage();

  printf("\n--- Multitasking Stress Test ---\n");

  task_create_user(&_start_user_task, &_end_user_task);

  printf("Main task (TID 0) entering infinite yield loop.\n");
  while (true) {
      task_yield(NULL);
  }
}
