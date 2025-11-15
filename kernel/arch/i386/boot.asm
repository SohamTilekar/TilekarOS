; Multiboot header constants.
MBALIGN  equ  1 << 0            ; Request page-aligned modules
MEMINFO  equ  1 << 1            ; Request memory map from bootloader
MBFLAGS  equ  MBALIGN | MEMINFO ; Combined flag field
MAGIC    equ  0x1BADB002        ; Multiboot magic signature
CHECKSUM equ -(MAGIC + MBFLAGS) ; Validates header (`MAGIC` + `FLAGS` + `CHECKSUM` == 0)

; Multiboot header section
; Must be located within the first 8 KiB of the kernel image at a 32-bit boundary.
; Bootloader parses this header to identify the image as a valid multiboot kernel.
section .multiboot
align 4
	dd MAGIC
	dd MBFLAGS
	dd CHECKSUM

; Kernel stack definition.
; Stack grows downward. Reserve 16 KiB for early kernel stack usage.
; The stack must be 16-byte aligned (System V ABI requirement).
section .bss
align 16
stack_bottom:
resb 16384
stack_top:

; Kernel entry point.
; `_start` is set in the linker script as the entry symbol.
; The bootloader transfers control here in 32-bit protected mode.
; At this stage: interrupts and paging are disabled, GDT not yet configured.
section .text
global _start:function (_start.end - _start)
_start:
	; Initialize stack pointer
	mov esp, stack_top
	extern init_gdt
	call init_gdt

	; TODO:
	; - Load GDT
	; - Initialize segment registers
	; - Enable paging if needed
	; - Setup hardware and runtime state before calling C entry point
	; Note: FPU/SSE and other CPU features are still uninitialized here.

	; Transfer control to the C/C++ kernel entry.
	; Stack alignment is preserved (16-byte aligned at call time).
	extern kernel_main
	call kernel_main

	; System halt loop.
	; Ensures CPU remains idle when kernel_main returns (should never happen).
	cli
.hang:
	hlt
	jmp .hang
.end:
