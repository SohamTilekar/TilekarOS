# TilekarOS Development Documentation

Welcome to the official development documentation for **TilekarOS**. This document provides an overview of the operating system's architecture, code flow, build system, and internal mechanisms.

## Project Overview

TilekarOS is a 32-bit (i386) hobby operating system written in C and Assembly. It follows the **Multiboot** specification, allowing it to be booted by compliant bootloaders like GRUB.

**Key Features:**
*   **Architecture**: x86 (i386)
*   **Boot Protocol**: Multiboot 1
*   **Kernel Mode**: Protected Mode (Ring 0)
*   **User Mode**: Ring 3 support (GDT/TSS setup)
*   **Interrupts**: Full IDT setup with PIC remapping and exception handling
*   **Standard Library**: Custom minimal `libc` implementation including:
    *   `stdio.h` (`printf`, `puts`, `putchar`)
    *   `string.h` (`memcpy`, `memset`, `strlen`, etc.)
    *   `stdlib.h` (`abort`)
*   **Tests**: Kernel currently runs a "Divide by Zero" test on boot to verify IDT exception handling.

## Documentation Index

*   [Architecture & Internals](./ARCHITECTURE.md) - Details on Boot Flow, GDT, IDT, and Memory.
*   [Build System](./BUILD_SYSTEM.md) - How to build, run, and debug the project.

## Directory Structure

The codebase is organized as follows:

*   **`kernel/`**: The core kernel source code.
    *   `arch/i386/`: Architecture-specific code (Bootloader, GDT, IDT, VGA).
    *   `include/`: Kernel-specific headers.
    *   `kernel.c`: Main kernel entry point.
*   **`libc/`**: Implementation of the standard C library (string, stdio, stdlib).
*   **`sysroot/`**: The virtual root filesystem where headers and compiled libraries are staged.
*   **`helpers/`**: Python scripts for build assistance (e.g., converting C defines to NASM).
*   **`isodir/`**: Staging area for generating the bootable ISO.
*   **`dump/`**: Temporary build artifacts (object files, binaries).

## How to Contribute

1.  **Modify Kernel**: Edit files in `kernel/`.
2.  **Modify LibC**: Edit files in `libc/`. If you add new files, ensure they are picked up by the `libc/Makefile`.
3.  **Build**: Run `make run_iso` to test your changes in QEMU.
4.  **Debug**: Use `make rund` to start QEMU in suspended mode waiting for a GDB connection.

## Future Roadmap
*   Physical Memory Manager (PMM)
*   Virtual Memory Manager (Paging)
*   Filesystem Support
*   Multitasking/Scheduling

## References & Resources

*   [OSDev.org Wiki](https://wiki.osdev.org/Main_Page) - The ultimate resource for OS development.
