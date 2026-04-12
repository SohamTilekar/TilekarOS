# 🌌 TilekarOS

TilekarOS is a hobbyist, 32-bit monolithic operating system built from scratch for the **x86 (i386)** architecture. Written primarily in **C** and **Assembly**, it serves as a platform for exploring kernel development, low-level hardware interfacing, and operating system design principles.

[![Documentation](https://img.shields.io/badge/docs-GitHub%20Pages-blue)](https://sohamtilekar.github.io/TilekarOS/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

---

## ✨ Key Features

- **🛡️ x86 Protected Mode**: Operates in 32-bit Protected Mode with custom GDT and IDT.
- **🔄 Multitasking & ELF**: Preemptive multitasking with a round-robin scheduler and **ELF32** executable loading.
- **📁 Storage & Filesystem**:
    - **VFS (Virtual File System)**: Abstracted file operations (`open`, `read`, `write`, `mkdir`).
    - **FAT12**: Native File Allocation Table implementation with read/write support.
    - **Buffer Cache**: Sector-level caching to accelerate disk I/O.
- **🔌 Hardware Drivers**:
    - **ATA (IDE)**: Support for hard disks using **PIO** and high-speed **DMA** modes.
    - **PCI Bus**: Automatic hardware discovery, enumeration, and Bus Master configuration.
    - **PS/2 Keyboard & VGA TTY**: Fully interactive console and input system.
- **🛠️ System Calls**: Robust API (via `int 0x80`) for user-space interaction.
- **📦 Custom LibC**: Minimal C standard library (`stdio`, `string`, `stdlib`).

---

## 🏗️ Project Structure

```text
.
├── kernel/                 # Kernel source code
├── libc/                   # Minimal C library implementation
├── sysroot/                # Generated userspace development environment
├── VirtualMachine/         # Default workspace for disk images and logs
├── cmake/toolchains/       # Cross-compilation configuration
└── Makefile                # Powerful master build & emulation control
```

---

## 🚀 Quick Start

### 🧰 Prerequisites

Ensure you have the following tools installed:
`Clang`, `LLD`, `NASM`, `CMake`, `Make`, `QEMU`, `grub-common`, `xorriso`, `mtools`.

### 🛠️ Build and Run

TilekarOS features a **Dynamic Workspace System** that manages isolated disk images and files.

```bash
# 1. Build kernel and populate development sysroot
make

# 2. Run in Kernel Mode (Fastest for kernel development)
make run DRIVES=boot:32:ahci

# 3. Run in Disk Mode (Realistic GRUB boot from a hard disk)
make run_disk DRIVES=boot:32:ide

# 4. Clean build artifacts
make clean
```

For a full list of commands and advanced configuration, run:
```bash
make help
```

---

## 💻 Userspace Development

TilekarOS supports a full userspace development environment via a **Sysroot**.

### 📦 The Sysroot
When you build the project, a `sysroot/` directory is automatically populated with:
- **Headers**: LibC and Kernel API headers in `sysroot/usr/include`.
- **Libraries**: `libc.a` static library in `sysroot/usr/lib`.
- **Startup**: `crt0.o` (entry point) and `user.ld` (linker script) in `sysroot/usr/lib`.

### 🛠️ Compiling Applications
You can compile your own C programs into OS-compatible binaries using the built-in helper:

```bash
# Compile a source file into an executable
make comp_exe FILE=./my_app.c OUT=./my_app
```

This uses the sysroot automatically to link against the correct standard library and headers for TilekarOS.

### 💿 Workspace & Persistence
TilekarOS manages disk images via **Workspaces** (Default: `VirtualMachine`).

1.  **Automatic Integration**: Any `.c` file placed in your workspace root is automatically copied to the boot drive root.
2.  **Persistence**: Files written by the OS to its disks are automatically exported to `$(VM)/exported_drives/` when you exit QEMU.
3.  **Two-Way Sync**: If you place files in `$(VM)/exported_drives/boot/`, they will be present on the OS disk at the next launch.

---

## 📚 Documentation

Comprehensive technical documentation is available at:
👉 **[sohamtilekar.github.io/TilekarOS/](https://sohamtilekar.github.io/TilekarOS/)**

---

## 📜 License

Distributed under the **MIT License**. See `LICENSE` for more information.
