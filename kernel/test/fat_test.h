#ifndef KERNEL_TEST_FAT_TEST_H
#define KERNEL_TEST_FAT_TEST_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "devices.h"
#include "fat.h"
#include "vfs.h"
#include "string.h"
#include "test_utils.h"

static inline bool fat_test_find_entry(const char* dir_path, const char* name) {
    int dirfd = vfs_open(dir_path, 0);
    if (dirfd < 0) return false;

    bool found = false;
    vfs_dirent_t ent;
    // Limit to 64 entries for safety in tests
    for (uint32_t i = 0; i < 64; i++) {
        if (vfs_readdir(dirfd, i, &ent) != 0) break;
        if (strcmp(ent.name, name) == 0) {
            found = true;
            break;
        }
    }
    vfs_close(dirfd);
    return found;
}

static inline void run_fat_tests(device_t* primary_storage, test_stats_t* stats) {
    // --- Core Initialization ---
    test_print_category(stats, "CORE INITIALIZATION");
    if (!primary_storage) {
        test_record(stats, false, "Primary storage device availability");
        return;
    }
    test_record(stats, true, "Primary storage device availability");

    fat_filesystem_t fs;
    test_record(stats, fat_init(&fs, primary_storage) == 0, "FAT filesystem metadata initialization");

    // --- Directory Operations ---
    test_print_category(stats, "DIRECTORY OPERATIONS");

    // We use /AUTOTEST as our root for tests
    int mk_root_res = vfs_mkdir("/AUTOTEST");
    test_record(stats, mk_root_res == 0 || mk_root_res == -2, "Create /AUTOTEST mount point");

    int mk_persist_res = vfs_mkdir("/AUTOTEST/PERSIST");
    test_record(stats, mk_persist_res == 0 || mk_persist_res == -2, "Create /AUTOTEST/PERSIST (To stay)");

    int mk_temp_res = vfs_mkdir("/AUTOTEST/TEMP");
    test_record(stats, mk_temp_res == 0 || mk_temp_res == -2, "Create /AUTOTEST/TEMP (To delete)");

    // --- File Operations ---
    test_print_category(stats, "FILE OPERATIONS");

    const char* p_data = "This file persists after kernel exit for manual check.";
    uint32_t p_len = (uint32_t)strlen(p_data);
    int p_res = fat_create_file(&fs, "/AUTOTEST/PERSIST/CHECK.TXT", (const uint8_t*)p_data, p_len);
    test_record(stats, p_res == 0, "Create /AUTOTEST/PERSIST/CHECK.TXT");

    const char* t_data = "TilekarOS FAT self-test temporary payload.";
    uint32_t t_len = (uint32_t)strlen(t_data);
    int t_res = fat_create_file(&fs, "/AUTOTEST/TEMP/UNLINK.TXT", (const uint8_t*)t_data, t_len);
    test_record(stats, t_res == 0, "Create /AUTOTEST/TEMP/UNLINK.TXT");

    // --- VFS Integration ---
    test_print_category(stats, "VFS INTEGRATION & READBACK");

    uint8_t readback[128];
    memset(readback, 0, sizeof(readback));
    int fd = vfs_open("/AUTOTEST/TEMP/UNLINK.TXT", 0);
    test_record(stats, fd >= 0, "vfs_open temporary file");

    if (fd >= 0) {
        int read_bytes = vfs_read(fd, readback, sizeof(readback));
        vfs_close(fd);
        bool match = (read_bytes == (int)t_len && memcmp(readback, t_data, t_len) == 0);
        test_record(stats, match, "vfs_read payload verification");
    } else {
        test_record(stats, false, "vfs_read payload verification (FD invalid)");
    }

    test_record(stats, fat_test_find_entry("/AUTOTEST/TEMP", "UNLINK.TXT"), "vfs_readdir finds UNLINK.TXT");
    test_record(stats, fat_test_find_entry("/AUTOTEST", "PERSIST"), "vfs_readdir finds PERSIST directory");

    // --- Cleanup & Persistence ---
    test_print_category(stats, "CLEANUP & PERSISTENCE");

    // We only unlink the temporary file to leave others for manual check
    int unlink_res = vfs_unlink("/AUTOTEST/TEMP/UNLINK.TXT");
    test_record(stats, unlink_res == 0, "vfs_unlink /AUTOTEST/TEMP/UNLINK.TXT");

    bool still_there = fat_test_find_entry("/AUTOTEST/TEMP", "UNLINK.TXT");
    test_record(stats, !still_there, "Verify UNLINK.TXT was successfully removed");

    /*
     * CLEANUP SECTION (Commented out for manual verification)
     * To fully clean up the disk, you would uncomment these:
     *
     * vfs_unlink("/AUTOTEST/PERSIST/CHECK.TXT");
     * // vfs_rmdir is not yet implemented for FAT
     * // vfs_rmdir("/AUTOTEST/PERSIST");
     * // vfs_rmdir("/AUTOTEST/TEMP");
     * // vfs_rmdir("/AUTOTEST");
     */

    test_print_divider(62);
    printf("| PERSISTENT ARTIFACTS LEFT FOR MANUAL CHECK                 |\n");
    printf("|  -> /AUTOTEST/PERSIST/CHECK.TXT                            |\n");
    printf("|  -> /AUTOTEST/TEMP/ (Empty Directory)                      |\n");
    test_print_divider(62);
}

#endif
