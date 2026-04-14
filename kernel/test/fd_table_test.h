#ifndef KERNEL_TEST_FD_TABLE_TEST_H
#define KERNEL_TEST_FD_TABLE_TEST_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "devices.h"
#include "fat.h"
#include "vfs.h"
#include "task.h"
#include "string.h"
#include "fat_test.h"

static inline void fd_table_test_idle_task(void) {
}

static inline void run_fd_table_tests(device_t* primary_storage, test_stats_t* stats) {
    test_print_category(stats, "PER-TASK FILE TABLE");

    test_record(stats, current_task != NULL, "current_task exists");
    if (!current_task) return;

    test_record(stats,
        current_task->file_table[0] != NULL &&
        current_task->file_table[1] != NULL &&
        current_task->file_table[2] != NULL,
        "main task has stdin/stdout/stderr initialized");

    task_t* created = task_create(fd_table_test_idle_task, 0);
    test_record(stats, created != NULL, "task_create returns a task");
    if (!created) return;

    test_record(stats,
        created->file_table[0] != NULL &&
        created->file_table[1] != NULL &&
        created->file_table[2] != NULL,
        "new task has stdin/stdout/stderr initialized");

    test_record(stats,
        created->file_table[0] != current_task->file_table[0] &&
        created->file_table[1] != current_task->file_table[1] &&
        created->file_table[2] != current_task->file_table[2],
        "stdio entries are not shared across tasks");

    if (!primary_storage) {
        test_record(stats, false, "primary storage available for FD isolation test");
        return;
    }

    fat_filesystem_t fs;
    if (fat_init(&fs, primary_storage) != 0) {
        test_record(stats, false, "FAT init for FD table test");
        return;
    }

    int mk_autotest = vfs_mkdir("/AUTOTEST");
    test_record(stats, mk_autotest == 0 || mk_autotest == -2, "ensure /AUTOTEST exists");

    const char* data = "fd-table-isolation";
    int create_res = fat_create_file(&fs, "/AUTOTEST/FDTABLE.TXT", (const uint8_t*)data, (uint32_t)strlen(data));
    test_record(stats, create_res == 0 || create_res == -2, "create /AUTOTEST/FDTABLE.TXT");

    int fd = vfs_open("/AUTOTEST/FDTABLE.TXT", 0);
    test_record(stats, fd >= 3, "main task opens test file");
    if (fd < 3) return;

    test_record(stats, created->file_table[fd] == NULL, "opened FD is isolated from new task table");

    int copy_res = task_file_table_copy(created, current_task);
    test_record(stats, copy_res == 0, "task_file_table_copy succeeds");
    if (copy_res == 0) {
        test_record(stats,
            created->file_table[fd] != NULL &&
            created->file_table[fd] != current_task->file_table[fd],
            "copy populates FD without sharing file object pointer");
    }

    int set_res = task_file_table_set(created, 7, current_task->file_table[1]);
    test_record(stats, set_res == 0, "task_file_table_set updates target FD");
    if (set_res == 0) {
        test_record(stats, created->file_table[7] != NULL, "target FD updated in task table");
    }

    vfs_close(fd);
}

#endif
