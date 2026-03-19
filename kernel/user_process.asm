[BITS 32]

section .text
global _start_user_task
global _end_user_task

_start_user_task:
    ; 1. Create directory /TESTDIR
    mov eax, 6          ; SYS_MKDIR
    call .get_dir_path
    db "/TESTDIR", 0
.get_dir_path:
    pop ebx
    int 0x80

    ; 2. Open root directory /
    mov eax, 3          ; SYS_OPEN
    call .get_root_path
    db "/", 0
.get_root_path:
    pop ebx
    mov ecx, 0
    int 0x80
    push eax            ; Save root FD

    ; 3. List root directory
    mov eax, 1          ; SYS_WRITE
    mov ebx, 1
    call .get_msg_list_root
    db "Root Directory Listing:", 10, 0
.get_msg_list_root:
    pop ecx
    mov edx, 24
    int 0x80

    sub esp, 264        ; vfs_dirent_t buffer
    mov edi, 0          ; index
.loop_root:
    mov eax, 9          ; SYS_READDIR
    mov ebx, [esp + 264] ; root FD
    mov ecx, edi        ; index
    mov edx, esp        ; out buffer
    int 0x80
    cmp eax, 0
    jne .done_root
    
    ; Print entry name
    mov eax, 1          ; SYS_WRITE
    mov ebx, 1
    mov ecx, esp        ; dirent->name
    mov edx, 256        ; max name len
    int 0x80
    
    mov eax, 1          ; SYS_WRITE
    mov ebx, 1
    call .get_newline
    db 10
.get_newline:
    pop ecx
    mov edx, 1
    int 0x80

    inc edi
    jmp .loop_root
.done_root:
    add esp, 264

    ; 4. Close root FD
    mov eax, 5          ; SYS_CLOSE
    pop ebx
    int 0x80

    ; 5. Delete directory /TESTDIR (using unlink for now as rmdir is alias)
    ; Actually, let's just exit
    mov eax, 0          ; SYS_EXIT
    int 0x80

_end_user_task:
