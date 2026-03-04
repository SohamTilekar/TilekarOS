; Multiboot header constants.
MULBOOT_PAGE_ALIGN  equ  1 << 0            ; Request page-aligned modules
MULBOOT_MEMORY_INFO  equ  1 << 1            ; Request memory map from bootloader
MULBOOT_HEADER_FLAGS  equ  MULBOOT_PAGE_ALIGN | MULBOOT_MEMORY_INFO ; Combined flag field
MULBOOT_HEADER_MAGIC    equ  0x1BADB002        ; Multiboot magic signature
MULBOOT_HEADER_CHECKSUM equ -(MULBOOT_HEADER_MAGIC + MULBOOT_HEADER_FLAGS) ; Validates header (`MAGIC` + `FLAGS` + `CHECKSUM` == 0)

; Multiboot header section
; Must be located within the first 8 KiB of the kernel image at a 32-bit boundary.
; Bootloader parses this header to identify the image as a valid multiboot kernel.
section .multiboot
align 4
	dd MULBOOT_HEADER_MAGIC
	dd MULBOOT_HEADER_FLAGS
	dd MULBOOT_HEADER_CHECKSUM

	dd 0, 0, 0, 0, 0

	dd 0
	dd 1024
	dd 768
	dd 32

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
section .boot
global _start:function (_start.end - _start)
_start:
    ; Initilizing Paging
    MOV ecx, (initial_page_dir - 0xC0000000)
    MOV cr3, ecx

    MOV ecx, cr4
    OR ecx, 0x10
    MOV cr4, ecx

    MOV ecx, cr0
    OR ecx, 0x80000000
    MOV cr0, ecx

    jmp higher_half_kernel
.end:

section .text
higher_half_kernel:
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
halt:
global halt:function (halt.end - halt)
	cli
.mh:
    hlt
	jmp .mh
.end:

section .data
align 4096
global initial_page_dir
initial_page_dir:
    DD 10000111b
    TIMES 768-1 DD 0

    DD (0 << 22) | 10000111b
    DD (1 << 22) | 10000111b
    DD (2 << 22) | 10000111b
    DD (3 << 22) | 10000111b
    TIMES 256-4 DD 0
