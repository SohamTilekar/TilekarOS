#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>

int global_var = 100;
volatile int sigchld_received = 0;

void sigchld_handler(int sig) {
    (void)sig;
    sigchld_received = 1;
}

void log_checkpoint(const char* id) {
    printf("[CHECKPOINT: %s]\n", id);
    int fd = open("/tmp/PROCESS.LOG", O_WRONLY | O_CREAT);
    if (fd >= 0) {
        // Simple append by reading till end
        char junk[256];
        while (read(fd, junk, sizeof(junk)) > 0);

        write(fd, id, strlen(id));
        write(fd, "\n", 1);
        close(fd);
    }
}

void test_memory_isolation() {
    printf("\n--- Memory Isolation Test ---\n");
    log_checkpoint("MEM_ISO_START");
    int local_var = 50;

    int pid = fork();
    if (pid == 0) {
        // Child process
        global_var = 200;
        local_var = 150;
        log_checkpoint("CHILD_MOD_VARS");
        exit(0);
    } else if (pid > 0) {
        // Parent process
        int status;
        waitpid(pid, &status, 0);
        printf("  PARENT: global_var is %d (expected 100), local_var is %d (expected 50)\n", global_var, local_var);
        log_checkpoint("PARENT_CHECK_VARS");

        if (global_var == 100 && local_var == 50) {
            log_checkpoint("MEM_ISO_PASS");
        }
    }
}

void test_waitpid_logic() {
    printf("\n--- Waitpid & Exit Status Test ---\n");
    log_checkpoint("WAITPID_START");

    int pid = fork();
    if (pid == 0) {
        printf("  CHILD: Exiting with status 42...\n");
        exit(42);
    } else {
        int status = 0;
        int reaped = waitpid(pid, &status, 0);
        printf("  PARENT: Reaped PID %d, status %d\n", reaped, status);

        if (reaped == pid && status == 42) {
            log_checkpoint("WAITPID_PASS");
        }
    }
}

void test_sigchld_delivery() {
    printf("\n--- SIGCHLD Delivery Test ---\n");
    log_checkpoint("SIGCHLD_START");

    signal(SIGCHLD, sigchld_handler);
    sigchld_received = 0;

    int pid = fork();
    if (pid == 0) {
        exit(0);
    } else {
        // Busy wait for signal
        for(int i=0; i<1000 && !sigchld_received; i++) yield();

        if (sigchld_received) {
            printf("  PARENT: Received SIGCHLD!\n");
            log_checkpoint("SIGCHLD_PASS");
        }

        waitpid(pid, NULL, 0); // Cleanup
    }
}

void test_nested_fork() {
    printf("\n--- Nested Fork (Grandchild) Test ---\n");
    log_checkpoint("NESTED_FORK_START");

    int pid = fork();
    if (pid == 0) {
        int gc_pid = fork();
        if (gc_pid == 0) {
            log_checkpoint("GRANDCHILD_ALIVE");
            exit(0);
        } else {
            waitpid(gc_pid, NULL, 0);
            log_checkpoint("CHILD_CREATED_GC");
            exit(0);
        }
    } else {
        waitpid(pid, NULL, 0);
    }
}

void test_execve_integration() {
    printf("\n--- Execve Integration Test ---\n");
    log_checkpoint("EXEC_START");

    execve("/BIN/HELLO", NULL, NULL);
    exit(1);
}

void test_multi_child_reap() {
    printf("\n--- Multi-Child Reaping Test ---\n");
    log_checkpoint("MULTI_CHILD_START");

    int pids[3];
    for (int i = 0; i < 3; i++) {
        pids[i] = fork();
        if (pids[i] == 0) {
            yield(); // Ensure parent forks all before exits
            exit(100 + i);
        }
    }

    // Reap in specific order (reverse)
    for (int i = 2; i >= 0; i--) {
        int status = 0;
        int reaped = waitpid(pids[i], &status, 0);
        printf("  PARENT: Reaped index %d (PID %d), status %d\n", i, reaped, status);
        if (reaped != pids[i] || status != (100 + i)) {
            printf("  [FAIL] Multi-reap mismatch\n");
            return;
        }
    }
    log_checkpoint("MULTI_CHILD_PASS");
}

void test_orphan_adoption() {
    printf("\n--- Orphan Adoption (Reparenting) Test ---\n");
    log_checkpoint("ORPHAN_START");

    int pid = fork();
    if (pid == 0) {
        // Child: creates a grandchild then dies
        int gc_pid = fork();
        if (gc_pid == 0) {
            // Grandchild: Wait for parent (child) to die
            log_checkpoint("GC_WAIT_REPARENT");
            for(int i=0; i<50; i++) yield();

            // In TilekarOS, orphans go to grandparent.
            // We can't easily get parent PID in current libc, but we can verify we didn't crash.
            log_checkpoint("GC_STILL_ALIVE");
            exit(77);
        }
        exit(0); // Child dies, GC is now orphan
    }

    // Parent reaps child
    waitpid(pid, NULL, 0);
    // Parent now waits for grandchild (who should be reparented to it)
    int status = 0;
    int reaped_gc = waitpid(-1, &status, 0);

    if (reaped_gc > 0 && status == 77) {
        printf("  PARENT: Successfully adopted and reaped grandchild %d\n", reaped_gc);
        log_checkpoint("ORPHAN_PASS");
    }
}

void test_stress_lifecycle() {
    printf("\n--- Stress: Rapid Fork/Exit/Wait (10 cycles) ---\n");
    for (int i = 0; i < 10; i++) {
        int pid = fork();
        if (pid == 0) exit(i);
        int status = -1;
        waitpid(pid, &status, 0);
        if (status != i) {
            printf("  [FAIL] Stress cycle %d failed\n", i);
            return;
        }
    }
    printf("  [PASS] 10 cycles complete.\n");
    log_checkpoint("STRESS_PASS");
}

int main() {
    printf("VIGOROUS POSIX PROCESS TESTS\n");
    printf("Base PID: %d\n", getpid());

    test_memory_isolation();
    test_waitpid_logic();
    test_sigchld_delivery();
    test_multi_child_reap();
    test_orphan_adoption();
    test_stress_lifecycle();
    test_nested_fork();

    printf("\nDispatching final tests...\n");
    test_execve_integration();
    return 0;
}
