#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int global_var = 100;

void test_memory_isolation() {
    printf("\n[TEST] Memory Isolation\n");
    int local_var = 50;

    uint32_t parent_pid = getpid();
    int fork_ret = fork();
    uint32_t current_pid = getpid();

    if (current_pid != parent_pid) {
        // Child process (PID 3)
        global_var = 200;
        local_var = 150;
        printf("CHILD:  My PID is %d (Parent was %d). Modified vars.\n", current_pid, parent_pid);
        exit(0);
    } else {
        // Parent process (PID 2)
        // Give child a chance to run
        printf("PARENT: My PID is still %d. Child PID was %d.\n", current_pid, fork_ret);
        printf("PARENT: global_var is %d (Expected 100), local_var is %d (Expected 50)\n", global_var, local_var);

        if (global_var == 100 && local_var == 50) printf("RESULT: Memory Isolation PASS\n");
        else printf("RESULT: Memory Isolation FAIL\n");
    }
}

void test_nested_fork() {
    printf("\n[TEST] Nested Fork (Grandchild)\n");

    uint32_t p_pid = getpid();
    int c_ret = fork();

    if (getpid() != p_pid) {
        // Child (PID 4)
        uint32_t c_pid = getpid();
        int g_ret = fork();

        if (getpid() != c_pid) {
            // Grandchild (PID 6)
            printf("GRANDCHILD: I am PID %d, child of %d\n", getpid(), c_pid);
            exit(0);
        } else {
            // Child (PID 4)
            // Wait for grandchild to print
            printf("CHILD: I am PID %d, created grandchild %d\n", c_pid, g_ret);
            exit(0);
        }
    }
    // Parent (PID 2)
}

void test_execve_integration() {
    printf("\n[TEST] Execve Integration\n");
    printf("PROCESS: I am PID %d, attempting to execve /BIN/HELLO...\n", getpid());
    int rc = execve("/BIN/HELLO", NULL, NULL);
    printf("PROCESS: ERROR! execve failed (rc=%d).\n", rc);
    exit(1);
}

int main() {
    printf("--- ADVANCED PROCESS LIFECYCLE TESTS ---\n");
    printf("Base PID: %d\n", getpid());

    test_memory_isolation();
    test_nested_fork();
    printf("\nPARENT: All tests dispatched. Waiting for children...\n");
    test_execve_integration();
    return 0;
}
