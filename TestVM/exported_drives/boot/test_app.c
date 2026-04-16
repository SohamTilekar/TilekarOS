#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int global_var = 100;

void test_memory_isolation() {
    printf("\n--- Memory Isolation Test ---\n");
    int local_var = 50;

    uint32_t parent_pid = getpid();
    int fork_ret = fork();
    uint32_t current_pid = getpid();

    if (current_pid != parent_pid) {
        // Child process
        global_var = 200;
        local_var = 150;
        printf("  CHILD: PID %d (Parent %d). Modified variables.\n", current_pid, parent_pid);
        exit(0);
    } else {
        // Parent process
        yield(); // Child Process should be run first
        printf("  PARENT: My PID %d. Child PID was %d.\n", current_pid, fork_ret);
        printf("  PARENT: global_var is %d (expected 100), local_var is %d(expected 50)\n", global_var, local_var);

        if (global_var == 100 && local_var == 50) {
            printf("  [PASS] Memory Isolation\n");
        } else {
            printf("  [FAIL] Memory Isolation\n");
        }
    }
}

void test_nested_fork() {
    printf("\n--- Nested Fork (Grandchild) Test ---\n");

    uint32_t p_pid = getpid();
    int c_ret = fork();

    if (getpid() != p_pid) {
        // Child
        uint32_t c_pid = getpid();
        int g_ret = fork();

        if (getpid() != c_pid) {
            // Grandchild
            printf("  GRANDCHILD: PID %d, child of %d\n", getpid(), c_pid);
            exit(0);
        } else {
            // Child
            printf("  CHILD: PID %d, created grandchild %d\n", c_pid, g_ret);
            exit(0);
        }
    }
    // Parent
}

void test_execve_integration() {
    printf("\n--- Execve Integration Test ---\n");
    printf("  PROCESS: PID %d, attempting execve /BIN/HELLO...\n", getpid());

    int rc = execve("/BIN/HELLO", NULL, NULL);

    // If we reach here, execve failed
    printf("  [FAIL] Execve failed (rc=%d)\n", rc);
    exit(1);
}

int main() {
    printf("ADVANCED PROCESS LIFECYCLE TESTS (USERSPACE)\n");
    printf("Base PID: %d\n", getpid());

    test_memory_isolation();
    test_nested_fork();

    printf("\nDispatching final tests...\n");
    test_execve_integration();
    return 0;
}
