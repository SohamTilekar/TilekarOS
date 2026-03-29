# TilekarOS Documentation

Welcome to the TilekarOS documentation. This project is a 32-bit hobby operating system designed for educational purposes.

## 🚀 Quick Start

Get TilekarOS up and running in 3 commands (assuming prerequisites are met):

```bash
make configure      # 1. Generate build files
make iso            # 2. Build the OS and create ISO
make run_iso        # 3. Launch in QEMU
```

---

## 🗺️ Documentation Map

The main entry point for the kernel is [kernel.c](https://github.com/SohamTilekar/TilekarOS/blob/main/kernel/kernel.c){: target="_blank" }.

### 🛠️ Getting Started
- [Build System](./getting-started/build-system.md): How to compile and run the OS.
- [Installation Guide](./getting-started/installation.md): Setting up the development environment.
- [Experimenting & Messing Around](./getting-started/experimenting.md): How to create tasks, use the VFS, and trigger failures.

### 🧠 Kernel Internals (Deep Dive)
- [Boot & Initialization](./kernel/boot-and-init.md): Detailed startup sequence from GRUB to `kernel_main`.
- [CPU & Interrupts](./kernel/cpu-internals.md): GDT, IDT, and low-level hardware control.
- [Memory Management](./kernel/memory-management.md): PMM, Recursive Paging, and Heap internals.
- [Multitasking & ELF](./kernel/multitasking.md): Context switching and binary loading.
- [Filesystem Stack](./kernel/filesystem.md): VFS, FAT12, and Buffer Cache architecture.

### 🔌 Drivers & API
- [Driver Reference](./drivers/driver-reference.md): Detailed specs for Keyboard, Timer, and VGA.
- [System Call Reference](./api/syscall-reference.md): How to use kernel services from user-land.
- [LibC Internals](./api/libc-internals.md): Implementation details of standard C functions.
- [LibC Reference](./api/libc-reference.md): Function signatures and usage.

### 👷 Development & Contributions
- [Coding Style](./development/coding-style.md): Best practices for writing kernel code.
- [Documentation Guide](./development/documentation-guide.md): How to contribute to this documentation.

---

## 🛠️ Troubleshooting

| Issue | Potential Solution |
| :--- | :--- |
| **`grub-mkrescue: not found`** | Install `grub-pc-bin` and `xorriso`. |
| **`mformat: command not found`** | Install `mtools`. |
| **`qemu-system-i386: command not found`** | Install `qemu-system-x86`. |
| **Build fails with `clang` errors** | Ensure you have `lld` and `clang` installed. |

---

## 📊 Technical Specs
- **Architecture**: i386 (32-bit x86)
- **Bootloader**: Multiboot-compliant (e.g., GRUB)
- **Memory Model**: Higher-half (starts at 0xC0000000), Paged (4KB / 4MB Pages)
- **Executable Format**: ELF32 (Supported)
- **Storage**: VFS with FAT12 Support
