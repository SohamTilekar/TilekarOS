[bits 32]

section .text

global _start_user_task
global _end_user_task
global user_task_function

_start_user_task:
user_task_function:
    jmp .start_user_task_code

.user_msg:
    db "User task copied & running in Ring 3!", 10, 0

.start_user_task_code:
    ; Attempt to access kernel memory (should cause Page Fault)
    ; mov eax, [0xC0000000]

    ; sys_write(fd=1, buf=.user_msg, len=38)
    mov eax, 1          ; SYS_WRITE
    mov ebx, 1          ; fd
    call .get_eip
.get_eip:
    pop ecx
    sub ecx, (.get_eip - .user_msg) ; ecx now points to .user_msg position independent
    mov edx, 38         ; len
    int 0x80

    ; sys_exit
    mov eax, 0          ; SYS_EXIT
    int 0x80
.loop:
    jmp .loop

_end_user_task:
