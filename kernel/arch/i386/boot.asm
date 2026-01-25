; Multiboot header constants.
MULTIBOOT_PAGE_ALIGN  equ  1 << 0            ; Request page-aligned modules
MULTIBOOT_MEMORY_INFO  equ  1 << 1            ; Request memory map from bootloader
MULTIBOOT_HEADER_FLAGS  equ  MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO ; Combined flag field
MULTIBOOT_HEADER_MAGIC    equ  0x1BADB002        ; Multiboot magic signature
MULTIBOOT_HEADER_CHECKSUM equ -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS) ; Validates header (`MAGIC` + `FLAGS` + `CHECKSUM` == 0)

; Multiboot header section
; Must be located within the first 8 KiB of the kernel image at a 32-bit boundary.
; Bootloader parses this header to identify the image as a valid multiboot kernel.
section .multiboot
align 4
	dd MULTIBOOT_HEADER_MAGIC
	dd MULTIBOOT_HEADER_FLAGS
	dd MULTIBOOT_HEADER_CHECKSUM

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
	cli
	; Initialize stack pointer
	mov esp, stack_top

	; used later in kernel_main
	push ebx ; Push the pointer to the Multiboot information structure
	push eax ; Push the magic value

	extern init_kernel
	call init_kernel

	sti

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
