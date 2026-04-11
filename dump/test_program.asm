[BITS 32]
[SECTION .text]
GLOBAL _start

_start:
    ; sys_write(1, msg, msg_len)
    mov eax, 1      ; SYS_WRITE
    mov ebx, 1      ; fd 1 (stdout)
    mov ecx, msg
    mov edx, msg_len
    int 0x80

    ; sys_exit()
    mov eax, 0      ; SYS_EXIT
    int 0x80

[SECTION .data]
msg db "Hello from ELF on Disk!", 0x0A
msg_len equ $ - msg
