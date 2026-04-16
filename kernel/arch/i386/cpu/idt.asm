; -----------------------------------------------------------------------------
; IDT.ASM
; This file contains the low-level assembly wrappers for interrupt handling.
; It sets up the stack to match the 'InterruptReg_t' struct in C, switches
; to kernel segments, and calls the C functions.
; -----------------------------------------------------------------------------

section .text

; Import C functions from idt.c
extern isr_handler
extern irq_handler

; -----------------------------------------------------------------------------
; Function: idt_flush
; Description: Loads the IDT pointer into the CPU.
; C Prototype: void idt_flush(uint32_t);
; -----------------------------------------------------------------------------
global idt_flush
idt_flush:
    mov eax, [esp+4]   ; Get the pointer to the IDT pointer struct (passed as arg)
    lidt [eax]         ; Load the IDT register (LIDT instruction)
    sti                ; Turn interrupts back on (Set Interrupt Flag)
    ret

; -----------------------------------------------------------------------------
; MACRO: isr_no_err_stub
; Description: Handler for exceptions that DO NOT push an error code.
; We push a dummy '0' so the stack layout is consistent for the C handler.
; -----------------------------------------------------------------------------
%macro isr_no_err_stub 1
    global isr%1
    isr%1:
        cli                 ; Disable interrupts to prevent nesting confusion
        push long 0         ; Push dummy error code (to match struct layout)
        push long %1        ; Push the interrupt number
        jmp isr_common_stub ; Jump to the common handler code
%endmacro

; -----------------------------------------------------------------------------
; MACRO: isr_err_stub
; Description: Handler for exceptions that DO push an error code (like Page Fault).
; The CPU has already pushed the error code, so we don't push a dummy.
; -----------------------------------------------------------------------------
%macro isr_err_stub 1
    global isr%1
    isr%1:
        cli                 ; Disable interrupts
        ; (Error code is already on the stack by CPU)
        push long %1        ; Push the interrupt number
        jmp isr_common_stub ; Jump to the common handler code
%endmacro

; -----------------------------------------------------------------------------
; MACRO: IRQ
; Description: Handler for Hardware Interrupts (IRQs).
; %1 = IRQ Number (0-15)
; %2 = Remapped IDT Vector (32-47)
; -----------------------------------------------------------------------------
%macro IRQ 2
    global irq%1
    irq%1:
        cli                 ; Disable interrupts
        push long 0         ; Push dummy error code
        push long %2        ; Push the remapped interrupt number (e.g., 32 for IRQ0)
        jmp irq_common_stub ; Jump to common IRQ handler
%endmacro

; -----------------------------------------------------------------------------
; Interrupt Vector Instantiation
; -----------------------------------------------------------------------------

; --- CPU Exceptions (0-31) ---
isr_no_err_stub 0   ; Division by Zero
isr_no_err_stub 1   ; Debug
isr_no_err_stub 2   ; NMI
isr_no_err_stub 3   ; Breakpoint
isr_no_err_stub 4   ; Overflow
isr_no_err_stub 5   ; Bound Range
isr_no_err_stub 6   ; Invalid Opcode
isr_no_err_stub 7   ; Device Not Available
isr_err_stub    8   ; Double Fault (Has Error Code)
isr_no_err_stub 9   ; Coprocessor Segment Overrun
isr_err_stub    10  ; Invalid TSS (Has Error Code)
isr_err_stub    11  ; Segment Not Present (Has Error Code)
isr_err_stub    12  ; Stack-Segment Fault (Has Error Code)
isr_err_stub    13  ; General Protection Fault (Has Error Code)
isr_err_stub    14  ; Page Fault (Has Error Code)
isr_no_err_stub 15  ; Reserved
isr_no_err_stub 16  ; x87 FPU Exception
isr_err_stub    17  ; Alignment Check (Has Error Code)
isr_no_err_stub 18  ; Machine Check
isr_no_err_stub 19  ; SIMD Exception
isr_no_err_stub 20  ; Virtualization
isr_no_err_stub 21  ; Reserved
isr_no_err_stub 22  ; Reserved
isr_no_err_stub 23  ; Reserved
isr_no_err_stub 24  ; Reserved
isr_no_err_stub 25  ; Reserved
isr_no_err_stub 26  ; Reserved
isr_no_err_stub 27  ; Reserved
isr_no_err_stub 28  ; Reserved
isr_no_err_stub 29  ; Reserved
isr_err_stub    30  ; Security Exception (Has Error Code)
isr_no_err_stub 31  ; Reserved

; --- System Calls ---
; isr_no_err_stub 128 ; 0x80 (Linux syscall compatibility) - now using custom syscall_stub
isr_no_err_stub 177 ; Custom syscall

; --- Hardware Interrupts (IRQs) ---
IRQ   0, 32 ; Timer
IRQ   1, 33 ; Keyboard
IRQ   2, 34 ; Cascade
IRQ   3, 35 ; COM2
IRQ   4, 36 ; COM1
IRQ   5, 37 ; LPT2
IRQ   6, 38 ; Floppy
IRQ   7, 39 ; LPT1
IRQ   8, 40 ; CMOS RTC
IRQ   9, 41 ; Peripherals
IRQ  10, 42 ; Peripherals
IRQ  11, 43 ; Peripherals
IRQ  12, 44 ; Mouse
IRQ  13, 45 ; FPU
IRQ  14, 46 ; Primary HDD
IRQ  15, 47 ; Secondary HDD

; -----------------------------------------------------------------------------
; MACRO: COMMON_INT_STUB
; Description: The shared code that saves the full CPU state, calls the C function,
; and restores the state.
;
; Stack Layout when calling C handler (InterruptReg_t struct):
; [ESP]    -> GS
; [ESP+4]  -> FS
; [ESP+8]  -> ES
; [ESP+12] -> DS
; [ESP+16] -> EDI, ESI, EBP, ESP, EBX, EDX, ECX, EAX (pushed by pushad)
; [ESP+48] -> Interrupt Number
; [ESP+52] -> Error Code
; [ESP+56] -> EIP (Return Address) - Pushed by CPU
; [ESP+60] -> CS  (Code Segment)   - Pushed by CPU
; [ESP+64] -> EFLAGS               - Pushed by CPU
; -----------------------------------------------------------------------------
%macro COMMON_INT_STUB 2
    global %1
%1:
    ; 1. Save General Purpose Registers
    pushad                  ; Pushes eax, ecx, edx, ebx, esp, ebp, esi, edi

    ; 2. Save Segment Registers
    push ds
    push es
    push fs
    push gs

    ; 3. Load Kernel Data Segment
    ; We must ensure we are in a known state before calling C code.
    mov ax, 0x10            ; 0x10 is usually the Kernel Data Segment Offset
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; 4. Call C Handler
    push esp                ; Pass pointer to stack (InterruptReg_t*) as argument
    call %2                 ; Call the C function (isr_handler or irq_handler)
    add esp, 4              ; Clean up the argument (pointer) pushed above

    ; 5. Restore State
    pop gs
    pop fs
    pop es
    pop ds

    popad                   ; Restore general purpose registers

    ; 6. Clean up Error Code and ISR Number
    add esp, 8              ; Pop the 2 integers pushed by the stub macros

    ; 7. Return from Interrupt
    sti                     ; Re-enable interrupts
    iret                    ; Pops CS, EIP, EFLAGS, (and SS, ESP if privilege change)
%endmacro

; Create the actual common stub labels
COMMON_INT_STUB isr_common_stub, isr_handler
COMMON_INT_STUB irq_common_stub, irq_handler
