[bits 32]
section .text
global _start
extern main

_start:
    ; Call main. main returns in eax.
    call main

    ; Move main's return value into ebx (second argument for the syscall) which will be the status code
    mov ebx, eax
    ; Place the syscall number (SYS_EXIT) into eax and invoke int 0x80
    mov eax, 0       ; user's SYS_EXIT value (per request)
    int 0x80

    ; Fallback loop if exit fails
.halt:
    hlt
    jmp .halt

section .note.GNU-stack noalloc noexec nowrite progbits
