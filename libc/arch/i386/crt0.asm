[bits 32]
section .text
global _start
extern main

_start:
    ; Set up environment if needed (argc, argv, etc.)
    call main

    ; Exit syscall (assuming syscall 1 is exit)
    mov ebx, eax ; return value from main
    mov eax, 1   ; exit syscall number
    int 0x80

    ; Fallback loop if exit fails
.halt:
    hlt
    jmp .halt

section .note.GNU-stack noalloc noexec nowrite progbits
