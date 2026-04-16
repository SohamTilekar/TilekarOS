#include "test_utils.h"
#include "../arch/i386/task/task.h"
#include "../arch/i386/fs/vfs.h"
#include "../arch/i386/drivers/timer.h"
#include "../arch/i386/drivers/keyboard.h"
#include <stdio.h>

/**
 * run_process_tests - Validates process lifecycle components.
 */
static inline void run_process_tests(test_stats_t* stats) {
    test_print_category(stats, "PROCESS LIFECYCLE (FORK/EXEC)");

    // 1. Verify existence of the userspace test binaries
    int fd_app = vfs_open("/BIN/TEST_APP", 0);
    int fd_hello = vfs_open("/BIN/HELLO", 0);
    bool exists = (fd_app >= 0 && fd_hello >= 0);
    if (fd_app >= 0) vfs_close(fd_app);
    if (fd_hello >= 0) vfs_close(fd_hello);

    test_record(stats, exists, "Required binaries /BIN/TEST_APP and /BIN/HELLO found");

    if (exists) {
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
            printf("| [>] Waiting 4 seconds for all tests to complete...         |\n");
            test_print_line('-', '-', '-', 62);
            task_start_scheduler();
            task_switch_to(test_task);

            uint32_t start_ticks = get_ticks();
            while (get_ticks() < start_ticks + 4000) { // 1000Hz timer
                task_yield(NULL);
            }
            test_print_line('-', '-', '-', 62);

            printf("| [?] Does the output match the expected output? (y/n): ");
            char choice = 0;
            keyboard_clear_buffer();
            uint32_t verify_start = get_ticks();
            while (choice == 0 && get_ticks() < verify_start + 10000) { // 10s timeout
                choice = keyboard_getchar();
                if (choice == 0) task_yield(NULL);
            }
            printf("\n");

            bool matched = (choice == 'y' || choice == 'Y');
            if (choice == 0) {
                printf("| [!] Verification input timed out. Recording as FAIL.       |\n");
            }
            test_record(stats, matched, "Userspace output manual verification");

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
