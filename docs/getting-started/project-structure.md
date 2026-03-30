# Project Structure

This document provides an overview of the TilekarOS directory layout and where to find key components of the system.

## 📂 Directory Overview

```text
TilekarOS/
├── kernel/                 # 🧠 Core Kernel Source
│   ├── arch/i386/          # 📟 x86 Specific Implementation
│   │   ├── boot/           # 🚀 Entry point and Linker script
│   │   ├── cpu/            # 📟 GDT, IDT, and CPU control
│   │   ├── drivers/        # 🔌 Hardware Drivers
│   │   │   ├── ata.c       # 💾 ATA Disk Driver
│   │   │   ├── pci.c       # 🚌 PCI Bus Driver
│   │   │   ├── keyboard.c  # ⌨️ Keyboard Driver
│   │   │   └── tty.c       # 🖥️ TTY/VGA Console
│   │   ├── fs/             # 📁 Filesystem & VFS
│   │   │   ├── vfs.c       # 🌐 Virtual File System
│   │   │   ├── fat.c       # 📂 FAT16/32 Driver
│   │   │   └── buffer.c    # 💾 Block Buffer Cache
│   │   ├── mm/             # 💾 Memory Management (PMM, VMM, Heap)
│   │   ├── syscalls/       # 🔌 System Call Dispatcher
│   │   ├── task/           # 🔄 Multitasking & ELF Loader
│   │   └── utils/          # 🛠️ Kernel Utilities
│   └── include/            # 📂 Kernel-wide Headers
├── libc/                   # 📚 Minimal C Standard Library
├── docs/                   # 📖 Documentation (MkDocs)
├── helpers/                # 🛠️ Build and Helper Scripts
├── sysroot/                # 📁 Build System Root
├── VirtualMachine/         # 💻 Default VM Workspace (Generated)
│   └── drives/             # 📀 VM Disk Images (.img)
└── build/                  # 🏗️ Build Artifacts (Generated)
```

## 🛠️ Key Files

| File | Description |
| :--- | :--- |
| `Makefile` | The main build orchestration file. |
| `CMakeLists.txt` | Configuration for the CMake build system. |
| `grub.cfg` | Configuration for the GRUB bootloader. |
| `.clangd` | Configuration for the Clangd Language Server (Generated). |
| `LICENSE` | The project license (MIT). |

## 🚀 Key Entry Points

- **Kernel Entry (ASM)**: `kernel/arch/i386/boot/boot.asm` - The very first code executed by the bootloader.
- **Kernel Entry (C)**: `kernel/arch/i386/init_kernel.c` - Performs architecture-specific initialization.
- **Kernel Main**: `kernel/kernel.c` - The platform-independent kernel main loop.
- **Linker Script**: `kernel/arch/i386/boot/linker.ld` - Defines the memory layout of the kernel binary.

---

## 🛠️ Build System Hierarchy

TilekarOS uses a nested build system for modularity:
1.  **Top-level Makefile**: Manages the overall build process, VM workspaces, and dynamic drives.
2.  **CMake**: Manages the compilation of the kernel and libc, generating IDE configuration.
3.  **Toolchains**: `cmake/toolchains/*.cmake` define cross-compilation settings.
