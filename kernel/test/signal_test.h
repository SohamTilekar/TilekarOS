#ifndef KERNEL_TEST_SIGNAL_TEST_H
#define KERNEL_TEST_SIGNAL_TEST_H

#include "test_utils.h"
#include "../arch/i386/task/task.h"
#include "../arch/i386/fs/vfs.h"
#include "../arch/i386/drivers/timer.h"
#include <stdio.h>
#include <string.h>

/**
 * run_signal_tests - Validates signal delivery and sigreturn.
 */
static inline void run_signal_tests(test_stats_t* stats) {
    test_print_category(stats, "SIGNALS (KILL, SIGACTION, SIGRETURN)");

    int fd_app = vfs_open("/BIN/SIG_TEST", 0);
    bool exists = (fd_app >= 0);
    if (fd_app >= 0) vfs_close(fd_app);

    test_record(stats, exists, "Required binary /BIN/SIG_TEST found");

    if (exists) {
        vfs_unlink("/tmp/SIGNAL.RES");
        vfs_unlink("/tmp/SIGNAL.LOG");

        printf("| [>] Stopping scheduler for task creation...                |\n");
        task_stop_scheduler();

        printf("| [>] Launching /BIN/SIG_TEST...                             |\n");
        task_t* test_task = task_create_elf_from_file("/BIN/SIG_TEST", 3);

        if (test_task) {
            test_record(stats, true, "Signal test task created");

            printf("| [>] EXPECTED TEST SEQUENCE:                                |\n");
            printf("| 1. Basic Delivery: Register, kill, execute handler.        |\n");
            printf("| 2. Sigprocmask: Block signal, verify pending, unblock.     |\n");
            printf("| 3. Sig_Ign: Ignore fatal signal and survive.               |\n");
            printf("| 4. Process resumes and writes PASS to /tmp/SIGNAL.RES.     |\n");

            printf("| [>] Starting scheduler and switching to test task...       |\n");
            printf("| [>] Waiting 3 seconds for tests to complete...             |\n");
            test_print_line('-', '-', '-', 62);
            task_start_scheduler();
            task_switch_to(test_task);

            uint32_t start_ticks = get_ticks();
            while (get_ticks() < start_ticks + 1000) { // Wait ~1s
                task_yield(NULL);
            }
            test_print_line('-', '-', '-', 62);

            // Verify checkpoints in log
            const char* expected_checkpoints[] = {
                "SIG_POWER_START",
                "TEST_REG_START",
                "TEST_REG_PASS",
                "TEST_MASKING_START",
                "HANDLER_USR1_ENTER",
                "HANDLER_USR1_RECURSION_BLOCKED",
                "HANDLER_USR1_EXIT",
                "HANDLER_USR1_ENTER",
                "HANDLER_USR1_EXIT",
                "TEST_MASKING_PASS",
                "TEST_PENDING_START",
                "SIGNALS_QUEUED",
                "PENDING_VERIFIED",
                "HANDLER_USR1_ENTER",
                "HANDLER_USR1_EXIT",
                "HANDLER_USR2_ENTER",
                "HANDLER_USR2_EXIT",
                "TEST_PENDING_PASS",
                "TEST_IGN_START",
                "TEST_IGN_PASS",
                "TEST_RESETHAND_START",
                "HANDLER_ALRM_ENTER",
                "HANDLER_ALRM_EXIT",
                "TEST_RESETHAND_PASS",
                "TEST_FATAL_START",
                "CHILD_READY_TO_DIE",
                "PARENT_SURVIVED_CHILD_DEATH",
                "TEST_FATAL_PASS",
                "SIG_POWER_COMPLETE"
            };
            int num_checkpoints = 29;
            bool checkpoints_found[29] = {false};

            printf("| [>] Verifying checkpoints via /tmp/SIGNAL.LOG...     |\n");
            int log_fd = vfs_open("/tmp/SIGNAL.LOG", 0);
            if (log_fd >= 0) {
                char log_content[2048];
                memset(log_content, 0, sizeof(log_content));
                vfs_read(log_fd, log_content, sizeof(log_content) - 1);
                vfs_close(log_fd);

                for (int i = 0; i < num_checkpoints; i++) {
                    if (strstr(log_content, expected_checkpoints[i])) {
                        checkpoints_found[i] = true;
                        printf("| [+] Found: %-47s |\n", expected_checkpoints[i]);
                    } else {
                        printf("| [!] Missing: %-45s |\n", expected_checkpoints[i]);
                    }
                }
            }

            bool all_checkpoints = true;
            for (int i = 0; i < num_checkpoints; i++) {
                if (!checkpoints_found[i]) all_checkpoints = false;
            }

            test_record(stats, all_checkpoints, "Signal handler execution and context restore");

            // Check overall PASS/FAIL
            int res = test_verify_res_file("/tmp/SIGNAL.RES");
            bool passed = (res == 1) && all_checkpoints;

            test_record(stats, passed, "Signal test overall verification");

        } else {
            task_start_scheduler();
            test_record(stats, false, "Failed to create task for /BIN/SIG_TEST");
        }
    } else {
        printf("| [!] INSTRUCTION: To fully verify signals:                     |\n");
        printf("| [!] 1. Compile TestVM/exported_drives/boot/sig_test.c         |\n");
        printf("| [!] 2. Place it in /BIN/SIG_TEST on the boot disk.            |\n");
    }
}

#endif
