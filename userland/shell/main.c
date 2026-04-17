#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>

#define MAX_INPUT_LEN 256
#define MAX_ARGS 16
#define MAX_PATH_LEN 512

char cwd[MAX_PATH_LEN] = "/";

void print_prompt() {
    printf("[%s] # ", cwd);
}

void normalize_path(char* path) {
    if (!path) return;
    for (int i = 0; path[i]; i++) {
        if (path[i] == '\\') path[i] = '/';
    }
    // Remove trailing slash unless it's root
    size_t len = strlen(path);
    if (len > 1 && path[len-1] == '/') {
        path[len-1] = '\0';
    }
}

void get_abs_path(const char* rel_path, char* abs_path) {
    if (!rel_path || rel_path[0] == '/' || rel_path[0] == '\\') {
        strcpy(abs_path, rel_path ? rel_path : "/");
    } else if (strcmp(rel_path, ".") == 0) {
        strcpy(abs_path, cwd);
    } else {
        if (strcmp(cwd, "/") == 0) {
            sprintf(abs_path, "/%s", rel_path);
        } else {
            sprintf(abs_path, "%s/%s", cwd, rel_path);
        }
    }
    normalize_path(abs_path);
}

void builtin_cd(const char* path) {
    if (!path || strcmp(path, "~") == 0) {
        strcpy(cwd, "/");
        return;
    }

    if (strcmp(path, "..") == 0) {
        if (strcmp(cwd, "/") == 0) return;
        char* last_slash = strrchr(cwd, '/');
        if (last_slash == cwd) {
            strcpy(cwd, "/");
        } else {
            *last_slash = '\0';
        }
        return;
    }
    
    if (strcmp(path, ".") == 0) return;

    char abs_path[MAX_PATH_LEN];
    get_abs_path(path, abs_path);

    // Verify it's a directory
    int fd = open(abs_path, O_RDONLY);
    if (fd < 0) {
        printf("cd: %s: No such file or directory\n", abs_path);
        return;
    }
    close(fd);
    
    strcpy(cwd, abs_path);
}

int main() {
    char input[MAX_INPUT_LEN];
    char* args[MAX_ARGS];

    printf("TilekarOS Shell\n");

    while (1) {
        print_prompt();

        int len = 0;
        while (len < MAX_INPUT_LEN - 1) {
            char c;
            int r = read(0, &c, 1);
            if (r <= 0) continue;

            if (c == '\n' || c == '\r') {
                printf("\n");
                break;
            } else if (c == '\b' || c == 127) {
                if (len > 0) {
                    len--;
                }
                continue;
            } else {
                input[len++] = c;
            }
        }
        input[len] = '\0';

        if (len == 0) continue;

        // Tokenize input
        int arg_count = 0;
        char* token = strtok(input, " ");
        while (token != NULL && arg_count < MAX_ARGS - 1) {
            args[arg_count++] = token;
            token = strtok(NULL, " ");
        }
        args[arg_count] = NULL;

        if (strcmp(args[0], "exit") == 0) {
            break;
        } else if (strcmp(args[0], "cd") == 0) {
            builtin_cd(args[1]);
        } else if (strcmp(args[0], "pwd") == 0) {
            printf("%s\n", cwd);
        } else if (strcmp(args[0], "help") == 0) {
            printf("Available commands: ls, cat, cd, pwd, help, exit, or any /bin program\n");
        } else {
            // Resolve relative paths in arguments for binaries
            // If ls is called without args, default to cwd
            if (strcmp(args[0], "ls") == 0 && args[1] == NULL) {
                args[1] = cwd;
                args[2] = NULL;
            } else {
                for (int i = 1; args[i] != NULL; i++) {
                    if (args[i][0] != '/' && args[i][0] != '\\' && strcmp(args[i], ".") != 0 && strcmp(args[i], "..") != 0) {
                        char* resolved = malloc(MAX_PATH_LEN);
                        get_abs_path(args[i], resolved);
                        args[i] = resolved;
                    } else if (strcmp(args[i], ".") == 0 || strcmp(args[i], "..") == 0) {
                        char* resolved = malloc(MAX_PATH_LEN);
                        if (strcmp(args[i], ".") == 0) {
                            strcpy(resolved, cwd);
                        } else {
                            // Manual parent resolution for argument
                            strcpy(resolved, cwd);
                            char* last = strrchr(resolved, '/');
                            if (last == resolved) strcpy(resolved, "/");
                            else if (last) *last = '\0';
                        }
                        args[i] = resolved;
                    }
                }
            }

            // Execute as binary
            int pid = fork();
            if (pid == 0) {
                // Child
                char path[MAX_PATH_LEN];
                
                // If it contains a slash, treat as path
                if (strchr(args[0], '/') || strchr(args[0], '\\')) {
                    get_abs_path(args[0], path);
                    execve(path, args, NULL);
                } else {
                    // Try /bin/
                    sprintf(path, "/bin/%s", args[0]);
                    execve(path, args, NULL);
                }

                printf("sh: command not found: %s\n", args[0]);
                _exit(1);
            } else if (pid > 0) {
                // Parent: wait for child
                for (int i = 0; i < 30; i++) yield();
            } else {
                printf("fork failed\n");
            }
        }
    }

    return 0;
}
