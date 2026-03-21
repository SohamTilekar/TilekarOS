section .text

global syscall_stub
extern syscall_handler

syscall_stub:
    cli
    push dword 0    ; dummy error code
    push dword 128  ; interrupt number
    pushad
    push ds
    push es
    push fs
    push gs

    mov ax, 0x10   ; kernel data selector
    mov ds, ax
    mov es, ax

    push esp
    call syscall_handler
    add esp, 4

    pop gs
    pop fs
    pop es
    pop ds
    popad
    add esp, 8      ; remove dummy error code and int number
    sti
    iretd
