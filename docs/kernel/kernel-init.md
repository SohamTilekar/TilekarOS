# Kernel Initialization Sequence

This document provides a concise overview of the TilekarOS startup process.

## 1. Bootloader Phase (GRUB)
- GRUB loads the kernel into physical memory at the **1MB mark** (0x100000).
- It enables **Protected Mode (32-bit)** and the **A20 line**.
- It passes the **Multiboot Information Structure** (memory map, modules, etc.).

### Multiboot Header
Located within the first 8KB of the kernel image, this header is parsed by GRUB:
- **Magic Number**: `0x1BADB002`.
- **Flags**: 
    - Bit 0: Page-aligned modules.
    - Bit 1: Memory information (e.g., memory map from BIOS).
- **Checksum**: `-(magic + flags)`.

## 2. Low-Level Entry (`boot.asm`)
- **Stack Setup**: Point `ESP` to a reserved 16KB block in the `.bss` section. The stack is 16-byte aligned.
- **Initial Paging**: To bridge the gap before the higher-half kernel is ready, `boot.asm` sets up a temporary page directory (`initial_page_dir`):
    - **Identity Mapping**: Maps physical 0-4MB to virtual 0-4MB using a **4MB Large Page**.
    - **Higher-Half Mapping**: Maps physical 0-4MB to virtual `0xC0000000` (3GB).
- **PSE (Page Size Extensions)**: Bit 4 of CR4 is set to enable 4MB pages.
- **Enabling Paging**: Bit 31 of CR0 is set to enable the paging hardware.
- **Call `init_kernel`**: Control is passed to the C kernel via a long jump to `higher_half_kernel`.

## 3. Linker Layout (`linker.ld`)
The linker script organizes the kernel binary into several key sections:

- **`.multiboot`**: Placed at the very beginning for GRUB detection.
- **`.boot`**: Contains the low-level entry code that runs before paging is fully initialized.
- **`.text`**: The main executable code, starting at `0xC0000000` (virtual) but loaded at `0x00100000` (physical).
- **`.rodata`**: Read-only constants and string literals.
- **`.data`**: Initialized global and static variables.
- **`.bss`**: Uninitialized variables (zeroed by the linker/loader) and the kernel stack.
- **`_kernel_end`**: A symbol marking the end of the kernel image, used by the PMM to find free memory.

## 4. C Kernel Init (`init_kernel.c`)

The following subsystems are initialized in order:

1. **VGA/TTY**: Clears the screen and prepares for text output.
2. **GDT**: Replaces the GRUB-provided table with our own (Code, Data, and TSS).
3. **IDT**: Configures interrupt gates and remaps the PIC.
4. **Keyboard**: Sets up the PS/2 keyboard driver.
5. **Memory**:
   - Calculates the end of the kernel image and any loaded modules.
   - Initializes the **Physical Memory Manager (PMM)** and **Paging**.
   - Identity mapping is removed.
6. **Kernel Heap**: Initializes `kmalloc` with a 1MB pool.
7. **Scheduler**: Initializes the multitasking system and creates the first "Main" task.

## 4. Post-Initialization
Once `init_kernel` returns, the kernel enters its main execution loop (typically in `kernel.c`), where it can start user tasks or run system services.
