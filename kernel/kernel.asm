[bits 32]

section .data
msg db "IOPL = ", 0
outbuf db "00", 10, 0   ; two digits + newline

section .text
global _start

_start:

    ; --- Read EFLAGS ---
    pushfd
    pop eax

    ; --- Extract IOPL (bits 12-13) ---
    mov ebx, eax
    shr ebx, 12
    and ebx, 3          ; EBX = IOPL (0..3)

    ; --- Convert to ASCII "00".."03" ---
    mov byte [outbuf], '0'
    add bl, '0'
    mov [outbuf + 1], bl

    ; --- sys_write(stdout, msg, 7) ---
    mov eax, 4          ; SYS_write
    mov ebx, 1          ; fd = stdout
    mov ecx, msg
    mov edx, 7
    int 0x80

    ; --- sys_write(stdout, outbuf, 3) ---
    mov eax, 4
    mov ebx, 1
    mov ecx, outbuf
    mov edx, 3
    int 0x80

    ; --- exit ---
    mov eax, 1
    xor ebx, ebx
    int 0x80
