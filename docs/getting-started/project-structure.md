# Project Structure

This document provides an overview of the TilekarOS directory layout and where to find key components of the system.

## 📂 Directory Overview

```text
TilekarOS/
├── kernel/                 # 🧠 Core Kernel Source
│   ├── arch/i386/          # 📟 x86 Specific Implementation
│   │   ├── boot/           # 🚀 Entry point and Linker script
│   │   ├── cpu/            # 📟 GDT, IDT, and CPU control
│   │   ├── drivers/        # 🔌 Hardware Drivers (VGA, Keyboard, Timer)
│   │   ├── fs/             # 📁 Filesystem & VFS
│   │   ├── mm/             # 💾 Memory Management (PMM, VMM, Heap)
│   │   ├── syscalls/       # 🔌 System Call Dispatcher
│   │   ├── task/           # 🔄 Multitasking & ELF Loader
│   │   └── utils/          # 🛠️ Kernel Utilities
│   └── include/            # 📂 Kernel-wide Headers
├── libc/                   # 📚 Minimal C Standard Library
│   ├── include/            # 📂 LibC Headers (stdio.h, string.h, etc.)
│   ├── stdio/              # 📟 Standard I/O Implementation
│   ├── stdlib/             # 🛠️ Standard Library (abort, etc.)
│   └── string/             # 🧵 String Manipulation
├── docs/                   # 📖 Documentation (MkDocs)
├── scripts/                # 🛠️ Build and Helper Scripts
├── sysroot/                # 📁 Build System Root
├── isodir/                 # 💿 ISO Image Layout
└── build/                  # 🏗️ Build Artifacts (Generated)
```

## 🛠️ Key Files

| File | Description |
| :--- | :--- |
| `Makefile` | The main build orchestration file. |
| `CMakeLists.txt` | Configuration for the CMake build system. |
| `grub.cfg` | Configuration for the GRUB bootloader. |
| `.clangd` | Configuration for the Clangd Language Server. |
| `LICENSE` | The project license (MIT). |

## 🚀 Key Entry Points

- **Kernel Entry (ASM)**: `kernel/arch/i386/boot/boot.asm` - The very first code executed by the bootloader.
- **Kernel Entry (C)**: `kernel/arch/i386/init_kernel.c` - Performs architecture-specific initialization.
- **Kernel Main**: `kernel/kernel.c` - The platform-independent kernel main loop.
- **Linker Script**: `kernel/arch/i386/boot/linker.ld` - Defines the memory layout of the kernel binary.

---

## 🛠️ Build System Hierarchy

TilekarOS uses a nested build system for modularity:
1.  **Top-level Makefile**: Manages the overall build process (ISO creation, cleaning).
2.  **CMake**: Manages the compilation of the kernel and libc.
3.  **Kernel-specific Makefile**: (Optional) Handles specialized kernel build tasks.
