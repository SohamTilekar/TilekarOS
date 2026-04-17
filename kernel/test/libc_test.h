#ifndef KERNEL_TEST_LIBC_TEST_H
#define KERNEL_TEST_LIBC_TEST_H

#include "test_utils.h"
#include "../arch/i386/task/task.h"
#include "../arch/i386/fs/vfs.h"
#include "../arch/i386/drivers/timer.h"
#include "../arch/i386/drivers/keyboard.h"
#include <stdio.h>
#include <string.h>

/**
 * run_libc_userspace_tests - Runs the /BIN/LIBC_TEST userspace program.
 */
static inline void run_libc_userspace_tests(test_stats_t* stats) {
    test_print_category(stats, "USERSPACE LIBC TESTS (/BIN/LIBC_TEST)");

    // 1. Verify existence of the libc test binary
    int fd_libc = vfs_open("/BIN/LIBC_TEST", 0);
    bool exists = (fd_libc >= 0);
    if (fd_libc >= 0) vfs_close(fd_libc);

    test_record(stats, exists, "Binary /BIN/LIBC_TEST found");

    if (exists) {
        // Clean up previous results if any
        vfs_unlink("/tmp/LIBC.RES");

        printf("| [>] Stopping scheduler for task creation...                |\n");
        task_stop_scheduler();

        printf("| [>] Launching /BIN/LIBC_TEST...                            |\n");
        task_t* test_task = task_create_elf_from_file("/BIN/LIBC_TEST", 3);

        if (test_task) {
            test_record(stats, true, "LIBC_TEST task created");

            printf("| [>] Starting scheduler and switching to LIBC_TEST...       |\n");
            printf("| [>] Waiting 3 seconds for tests to complete...             |\n");
            test_print_line('-', '-', '-', 62);
            task_start_scheduler();
            task_switch_to(test_task);

            uint32_t start_ticks = get_ticks();
            while (get_ticks() < start_ticks + 1000) { // 1000Hz timer
                task_yield(NULL);
            }
            test_print_line('-', '-', '-', 62);

            // Automated Verification
            printf("| [>] Verifying results via /tmp/LIBC.RES...           |\n");
            int res = test_verify_res_file("/tmp/LIBC.RES");
            bool passed = (res == 1);

            if (res == 0) {
                 printf("| [!] Result file reports FAIL.                             |\n");
            } else if (res == -1) {
                printf("| [!] Automated check failed (missing/empty). Falling back. |\n");
                printf("| [?] Does the output show 'RESULT: libc_test PASS'? (y/n): ");
                char choice = 0;
                keyboard_clear_buffer();
                uint32_t verify_start = get_ticks();
                while (choice == 0 && get_ticks() < verify_start + 5000) { // 5s timeout
                    choice = keyboard_getchar();
                    if (choice == 0) task_yield(NULL);
                }
                printf("\n");
                passed = (choice == 'y' || choice == 'Y');
            }

            test_record(stats, passed, "LibC userspace verification");
        } else {
            task_start_scheduler();
            test_record(stats, false, "Failed to create task for /BIN/LIBC_TEST");
        }
    } else {
        printf("| [!] INSTRUCTION: To run libc userspace tests:                |\n");
        printf("| [!] 1. Compile TestVM/exported_drives/boot/libc_test.c       |\n");
        printf("| [!] 2. Place it in /BIN/LIBC_TEST on the boot disk.          |\n");
    }
}

#endif
