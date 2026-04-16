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

static inline bool fat_test_write_file(const char* path, const char* data) {
    int fd = vfs_open(path, VFS_O_CREAT);
    if (fd < 0) return false;
    uint32_t len = (uint32_t)strlen(data);
    int written = vfs_write(fd, data, len);
    vfs_close(fd);
    return written == (int)len;
}

static inline bool fat_test_readback_matches(const char* path, const char* expected) {
    uint8_t readback[256];
    memset(readback, 0, sizeof(readback));
    int fd = vfs_open(path, 0);
    if (fd < 0) return false;
    int read_bytes = vfs_read(fd, readback, sizeof(readback));
    vfs_close(fd);
    uint32_t expected_len = (uint32_t)strlen(expected);
    return (read_bytes == (int)expected_len && memcmp(readback, expected, expected_len) == 0);
}

static inline void run_fat_tests(Device_t* primary_storage, test_stats_t* stats) {
    test_print_category(stats, "CORE INITIALIZATION");
    if (!primary_storage) {
        test_record(stats, false, "Primary storage device availability");
        return;
    }
    test_record(stats, true, "Primary storage device availability");

    fat_filesystem_t fs;
    test_record(stats, fat_init(&fs, primary_storage) == 0, "FAT filesystem metadata initialization");

    test_print_category(stats, "DIRECTORY OPERATIONS");
    int mk_root_res = vfs_mkdir("/AUTOTEST");
    test_record(stats, mk_root_res == 0 || mk_root_res == -2, "Create /AUTOTEST mount point");

    int mk_persist_res = vfs_mkdir("/AUTOTEST/PERSIST");
    test_record(stats, mk_persist_res == 0 || mk_persist_res == -2, "Create /AUTOTEST/PERSIST (To stay)");

    int mk_temp_res = vfs_mkdir("/AUTOTEST/TEMP");
    test_record(stats, mk_temp_res == 0 || mk_temp_res == -2, "Create /AUTOTEST/TEMP (To delete)");

    test_print_category(stats, "VFS CREATE/WRITE/READ");
    const char* p_data = "Persistent write path check via VFS.";
    const char* t_data = "Temporary write path check via VFS.";

    // Ensure deterministic content for repeated test runs.
    vfs_unlink("/AUTOTEST/PERSIST/CHECK.TXT");
    vfs_unlink("/AUTOTEST/TEMP/UNLINK.TXT");

    test_record(stats, fat_test_write_file("/AUTOTEST/PERSIST/CHECK.TXT", p_data), "vfs_open(O_CREAT)+vfs_write persistent file");
    test_record(stats, fat_test_write_file("/AUTOTEST/TEMP/UNLINK.TXT", t_data), "vfs_open(O_CREAT)+vfs_write temp file");
    test_record(stats, fat_test_readback_matches("/AUTOTEST/PERSIST/CHECK.TXT", p_data), "vfs_read persistent file payload");
    test_record(stats, fat_test_readback_matches("/AUTOTEST/TEMP/UNLINK.TXT", t_data), "vfs_read temp file payload");

    test_record(stats, fat_test_find_entry("/AUTOTEST/TEMP", "UNLINK.TXT"), "vfs_readdir finds UNLINK.TXT");
    test_record(stats, fat_test_find_entry("/AUTOTEST", "PERSIST"), "vfs_readdir finds PERSIST directory");

    test_print_category(stats, "UNLINK + RMDIR");
    int unlink_res = vfs_unlink("/AUTOTEST/TEMP/UNLINK.TXT");
    test_record(stats, unlink_res == 0, "vfs_unlink /AUTOTEST/TEMP/UNLINK.TXT");
    test_record(stats, !fat_test_find_entry("/AUTOTEST/TEMP", "UNLINK.TXT"), "Verify UNLINK.TXT was successfully removed");

    // rmdir should reject non-empty directory.
    vfs_unlink("/AUTOTEST/TEMP/RMDIR_CASE/KEEP.TXT");
    vfs_rmdir("/AUTOTEST/TEMP/RMDIR_CASE");

    int mk_case = vfs_mkdir("/AUTOTEST/TEMP/RMDIR_CASE");
    test_record(stats, mk_case == 0 || mk_case == -2, "Create /AUTOTEST/TEMP/RMDIR_CASE");
    test_record(stats, fat_test_write_file("/AUTOTEST/TEMP/RMDIR_CASE/KEEP.TXT", "rmdir-non-empty-check"), "Create file inside RMDIR_CASE");

    int non_empty_rmdir = vfs_rmdir("/AUTOTEST/TEMP/RMDIR_CASE");
    test_record(stats, non_empty_rmdir != 0, "vfs_rmdir rejects non-empty directory");

    int cleanup_file = vfs_unlink("/AUTOTEST/TEMP/RMDIR_CASE/KEEP.TXT");
    test_record(stats, cleanup_file == 0, "Cleanup file inside RMDIR_CASE");

    int empty_rmdir = vfs_rmdir("/AUTOTEST/TEMP/RMDIR_CASE");
    test_record(stats, empty_rmdir == 0, "vfs_rmdir removes empty directory");
    test_record(stats, !fat_test_find_entry("/AUTOTEST/TEMP", "RMDIR_CASE"), "vfs_readdir confirms RMDIR_CASE removal");

    test_print_category(stats, "PERSISTENCE ARTIFACT");
    test_record(stats, fat_test_find_entry("/AUTOTEST/PERSIST", "CHECK.TXT"), "Persistent artifact remains for manual inspection");
}

#endif
