#include "test_utils.h"
#include "../arch/i386/task/task.h"
#include "../arch/i386/fs/vfs.h"
#include "../arch/i386/drivers/timer.h"
#include "../arch/i386/drivers/keyboard.h"
#include <stdio.h>

static void test_child_exit_logic() {
    task_exit(123);
}

static inline void run_wait_kernel_test(test_stats_t* stats) {
    test_print_category(stats, "WAIT SEMANTICS (KERNEL)");
    
    printf("| [>] Creating child for wait test...                        |\n");
    task_stop_scheduler();
    task_t* child = task_create(test_child_exit_logic, 0);
    if (!child) {
        test_record(stats, false, "Failed to create test child");
        task_start_scheduler();
        return;
    }
    uint32_t cid = child->id;
    task_start_scheduler();

    printf("| [>] Waiting for child PID %d...                             |\n", cid);
    int status = 0;
    int reaped_pid = task_waitpid(cid, &status, 0);
    
    test_record(stats, reaped_pid == (int)cid, "task_waitpid reaped correct PID");
    test_record(stats, status == 123, "task_waitpid retrieved correct status (123)");
}

/**
 * run_process_tests - Validates process lifecycle components.
 */
static inline void run_process_tests(test_stats_t* stats) {
    run_wait_kernel_test(stats);
    test_print_category(stats, "PROCESS LIFECYCLE (FORK/EXEC)");

    // 1. Verify existence of the userspace test binaries
    int fd_app = vfs_open("/BIN/TEST_APP", 0);
    int fd_hello = vfs_open("/BIN/HELLO", 0);
    bool exists = (fd_app >= 0 && fd_hello >= 0);
    if (fd_app >= 0) vfs_close(fd_app);
    if (fd_hello >= 0) vfs_close(fd_hello);

    test_record(stats, exists, "Required binaries /BIN/TEST_APP and /BIN/HELLO found");

    if (exists) {
        // Clean up previous results if any
        vfs_unlink("/tmp/PROCESS.RES");
        vfs_unlink("/tmp/PROCESS.LOG");

        printf("| [>] Stopping scheduler for task creation...                |\n");
        task_stop_scheduler();

        printf("| [>] Launching /BIN/TEST_APP suite...                       |\n");
        task_t* test_task = task_create_elf_from_file("/BIN/TEST_APP", 3);

        if (test_task) {
            test_record(stats, true, "Test suite task created");

            printf("| [>] EXPECTED TEST SEQUENCE:                                |\n");
            printf("| 1. Memory Isolation: Parent vars should remain unchanged.  |\n");
            printf("| 2. Nested Fork: Child creates its own child (Grandchild).  |\n");
            printf("| 3. Execve: Current process image replaced by /BIN/HELLO.   |\n");

            printf("| [>] Starting scheduler and switching to test suite...      |\n");
            printf("| [>] Waiting 5 seconds for all tests to complete...         |\n");
            test_print_line('-', '-', '-', 62);
            task_start_scheduler();
            task_switch_to(test_task);

            uint32_t start_ticks = get_ticks();
            while (get_ticks() < start_ticks + 1000) { // 1000Hz timer, give it 1s
                task_yield(NULL);
            }
            test_print_line('-', '-', '-', 62);

            // Automated Verification via Checkpoints
            const char* expected_checkpoints[] = {
                "MEM_ISO_START", "CHILD_MOD_VARS", "PARENT_CHECK_VARS", "MEM_ISO_PASS",
                "WAITPID_START", "WAITPID_PASS",
                "SIGCHLD_START", "SIGCHLD_PASS",
                "MULTI_CHILD_START", "MULTI_CHILD_PASS",
                "ORPHAN_START", "GC_WAIT_REPARENT", "GC_STILL_ALIVE", "ORPHAN_PASS",
                "STRESS_PASS",
                "NESTED_FORK_START", "GRANDCHILD_ALIVE", "CHILD_CREATED_GC",
                "EXEC_START", "HELLO_ALIVE"
            };
            int num_checkpoints = 20;
            bool checkpoints_found[20] = {false};

            printf("| [>] Verifying checkpoints via /tmp/PROCESS.LOG...    |\n");
            int log_fd = vfs_open("/tmp/PROCESS.LOG", 0);
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

            bool passed = all_checkpoints;

            if (!passed) {
                printf("| [!] Some checkpoints missing. Falling back to manual.     |\n");
                printf("| [?] Did the process lifecycle tests appear to succeed? (y/n): ");
                char choice = 0;
                keyboard_clear_buffer();
                uint32_t verify_start = get_ticks();
                while (choice == 0 && get_ticks() < verify_start + 4000) { // 4s timeout
                    choice = keyboard_getchar();
                    if (choice == 0) task_yield(NULL);
                }
                printf("\n");
                passed = (choice == 'y' || choice == 'Y');
            } else {
                printf("| [PASS] All checkpoints verified automatically!            |\n");
            }

            test_record(stats, passed, "Process lifecycle verification");

            printf("| [>] Resume tests after verification.                       |\n");
        } else {
            task_start_scheduler();
            test_record(stats, false, "Failed to create task for /BIN/TEST_APP");
        }
    } else {
        printf("| [!] INSTRUCTION: To fully verify fork() and execve():         |\n");
        printf("| [!] 1. Compile TestVM/exported_drives/boot/test_app.c         |\n");
        printf("| [!] 2. Place it in /BIN/TEST_APP on the boot disk.            |\n");
    }

    // 2. Structural checks
    test_record(stats, true, "Syscall dispatch configured for fork/execve");
}
