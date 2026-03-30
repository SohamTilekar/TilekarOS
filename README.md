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
│   ├── arch/i386/          # x86-specific architecture, drivers, and fs
│   └── include/            # Kernel-wide headers
├── libc/                   # Minimal C library implementation
├── docs/                   # Detailed technical documentation & guides
├── cmake/toolchains/       # Cross-compilation configuration
└── Makefile                # Powerful wrapper for build & emulation
```

---

## 🚀 Quick Start

### 🧰 Prerequisites

Ensure you have the following tools installed:
`Clang`, `LLD`, `NASM`, `CMake`, `Make`, `QEMU`, `grub-common`, `xorriso`.

### 🛠️ Build and Run

TilekarOS uses a **Dynamic Build System** that manages isolated VM workspaces and disk images.

```bash
# 1. Clone the repository
git clone https://github.com/sohamtilekar/TilekarOS.git
cd TilekarOS

# 2. Build and run in QEMU with a 10MB disk image
make run DRIVES=disk:10

# 3. Build and run a bootable ISO
make run_iso

# 4. Debug with GDB
make debug_run

# 5. Clean build artifacts and VM workspace
make clean VM=MyTestVM
```

---

## 📚 Documentation

Comprehensive technical documentation is available at:
👉 **[sohamtilekar.github.io/TilekarOS/](https://sohamtilekar.github.io/TilekarOS/)**

---

## 📜 License

Distributed under the **MIT License**. See `LICENSE` for more information.

