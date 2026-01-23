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
