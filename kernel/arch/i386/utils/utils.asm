; -----------------------------------------------------------------------------
; UTILS.ASM
; Low-level helper functions for i386 architecture.
; -----------------------------------------------------------------------------

section .text

; -----------------------------------------------------------------------------
; Function: out_port_b
; Description: Writes a byte to an I/O port.
; C Prototype: void out_port_b(uint16_t port, uint8_t value);
; -----------------------------------------------------------------------------
global out_port_b
out_port_b:
    push ebp
    mov ebp, esp

    ; Stack Layout:
    ; [ebp]    = Old EBP
    ; [ebp+4]  = Return Address
    ; [ebp+8]  = Argument 1 (port)
    ; [ebp+12] = Argument 2 (value)

    mov dx, [ebp+8]    ; Load port address into DX
    mov al, [ebp+12]   ; Load value into AL

    out dx, al         ; Output AL to port DX

    pop ebp
    ret

; -----------------------------------------------------------------------------
; Function: out_port_w
; Description: Writes a word to an I/O port.
; C Prototype: void out_port_w(uint16_t port, uint16_t value);
; -----------------------------------------------------------------------------
global out_port_w
out_port_w:
    push ebp
    mov ebp, esp

    mov dx, [ebp+8]    ; Load port address into DX
    mov ax, [ebp+12]   ; Load value into AX

    out dx, ax         ; Output AX to port DX

    pop ebp
    ret

; -----------------------------------------------------------------------------
; Function: in_port_w
; Description: Reads a word from an I/O port.
; C Prototype: uint16_t in_port_w(uint16_t port);
; -----------------------------------------------------------------------------
global in_port_w
in_port_w:
    push ebp
    mov ebp, esp

    mov dx, [ebp+8]    ; Load port address into DX
    in ax, dx          ; Read a word from port DX into AX

    pop ebp
    ret

; -----------------------------------------------------------------------------
; Function: out_port_l
; Description: Writes a double word (32-bit) to an I/O port.
; C Prototype: void out_port_l(uint16_t port, uint32_t value);
; -----------------------------------------------------------------------------
global out_port_l
out_port_l:
    push ebp
    mov ebp, esp

    mov dx, [ebp+8]    ; Load port address into DX
    mov eax, [ebp+12]  ; Load value into EAX

    out dx, eax         ; Output EAX to port DX

    pop ebp
    ret

; -----------------------------------------------------------------------------
; Function: in_port_l
; Description: Reads a double word (32-bit) from an I/O port.
; C Prototype: uint32_t in_port_l(uint16_t port);
; -----------------------------------------------------------------------------
global in_port_l
in_port_l:
    push ebp
    mov ebp, esp

    mov dx, [ebp+8]    ; Load port address into DX
    in eax, dx          ; Read a dword from port DX into EAX

    pop ebp
    ret

; -----------------------------------------------------------------------------
; Function: in_port_b
; Description: Reads a byte from an I/O port.
; C Prototype: uint8_t in_port_b(uint16_t port);
; -----------------------------------------------------------------------------
global in_port_b
in_port_b:
    push ebp
    mov ebp, esp

    ; Stack Layout:
    ; [ebp]    = Old EBP
    ; [ebp+4]  = Return Address
    ; [ebp+8]  = Argument 1 (port)

    mov dx, [ebp+8]    ; Load port address into DX
    in al, dx          ; Read a byte from port DX into AL

    pop ebp
    ret
