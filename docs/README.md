# TilekarOS Documentation

Welcome to the TilekarOS documentation. This project is a 32-bit hobby operating system designed for educational purposes.

## Documentation Map

### 1. Getting Started
- [Build System](./getting-started/build-system.md): How to compile and run the OS.
- [Installation Guide](./getting-started/installation.md): Setting up the development environment.
- [Experimenting & Messing Around](./getting-started/experimenting.md): How to create tasks and intentionally trigger failures.

### 2. Kernel Internals (for OS Developers)
- [Architecture Overview](./kernel/architecture.md): Boot sequence and memory layout.
- [Kernel Initialization](./kernel/kernel-init.md): Detailed startup sequence from GRUB to `kernel_main`.
- [GDT & TSS](./kernel/gdt.md): Global Descriptor Table and segment management.
- [Memory Management](./kernel/memory-management.md): Physical Memory (PMM), Virtual Memory (VMM), and Heap (`kmalloc`).
- [Interrupts & Exceptions](./kernel/interrupts.md): IDT, PIC, and IRQ handling.
- [Multitasking](./kernel/multitasking.md): Context switching assembly and scheduler logic.
- [Task Management Guide](./kernel/tasks-guide.md): Creating and managing kernel and user tasks.
- [System Call Dispatcher](./kernel/syscalls.md): How the kernel handles `int 0x80`.
- [Kernel Utilities](./kernel/kernel-utilities.md): Low-level helpers (I/O ports, interrupt control, `InteruptReg`).

### 3. Drivers
- [VGA/TTY](./drivers/vga-tty.md): Text mode output and terminal logic.
- [Keyboard](./drivers/keyboard.md): PS/2 Keyboard and scancode handling.
- [Timer](./drivers/timer.md): PIT (Programmable Interval Timer).

### 4. API Reference (for Users/Programmers)
- [System Call Reference](./api/syscall-reference.md): How to use the kernel services.
- [LibC Reference](./api/libc-reference.md): Available C standard library functions.

### 5. Development & Contributions
- [Coding Style](./development/coding-style.md): Best practices for writing kernel code.
- [Documentation Guide](./development/documentation-guide.md): How to contribute to this documentation.

---

## Technical Specs
- **Architecture**: i386 (32-bit x86)
- **Bootloader**: Multiboot-compliant (e.g., GRUB)
- **Memory Model**: Flat / Paged
- **Kernel Space**: Higher-half (starts at 0xC0000000)
