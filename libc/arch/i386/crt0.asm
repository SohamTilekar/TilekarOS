[bits 32]
section .text
global _start
global __sig_restorer
extern main

_start:
    ; The kernel should have pushed argc and argv onto the stack
    ; [esp] = argc
    ; [esp+4] = argv
    
    push dword [esp+4] ; push argv
    push dword [esp+4] ; push argc (it was at esp+4 before first push)
    
    call main

    ; Main returns in eax, move to ebx as exit status
    mov ebx, eax
    ; Place the syscall number (SYS_EXIT) into eax and invoke int 0x80
    mov eax, 0       ; user's SYS_EXIT value (per request)
    int 0x80

    ; Fallback loop if exit fails
.halt:
    hlt
    jmp .halt

__sig_restorer:
    mov eax, 16      ; SYS_SIGRETURN = 16
    int 0x80

section .note.GNU-stack noalloc noexec nowrite progbits
