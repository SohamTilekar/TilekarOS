# 🌌 TilekarOS

TilekarOS is a hobbyist, 32-bit monolithic operating system built from scratch for the **x86 (i386)** architecture. Written primarily in **C** and **Assembly**, it serves as a platform for exploring kernel development, low-level hardware interfacing, and operating system design principles.

[![Documentation](https://img.shields.io/badge/docs-GitHub%20Pages-blue)](https://sohamtilekar.github.io/TilekarOS/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

---

## ✨ Key Features

- **🛡️ x86 Protected Mode**: Operates in 32-bit Protected Mode with a custom Global Descriptor Table (GDT) and Task State Segment (TSS).
- **📟 Interrupt Handling**: Robust Interrupt Descriptor Table (IDT) with handlers for CPU exceptions (Divide-by-Zero, Page Fault, etc.) and hardware IRQs.
- **🧠 Memory Management**:
    - **Physical Memory Manager (PMM)**: Uses a bitmap to track and allocate 4KB physical page frames.
    - **Paging**: Implements Page Directories and Page Tables for virtual memory management.
    - **Kernel Heap**: A dynamic memory allocator (`kmalloc`/`kfree`) for internal kernel use.
- **🔄 Multitasking**:
    - Preemptive multitasking using a round-robin scheduler triggered by the Programmable Interval Timer (PIT).
    - Support for Kernel-mode (Ring 0) and User-mode (Ring 3) tasks.
- **⌨️ Hardware Drivers**:
    - **VGA TTY**: A text-mode terminal driver with support for scrolling and basic ANSI-like colors.
    - **PS/2 Keyboard**: Interrupt-driven keyboard driver with scancode-to-ASCII mapping.
    - **PIT**: System timer for scheduling and delays.
- **📦 Custom Standard Library**: A minimal `libc` implementation including `stdio.h`, `string.h`, and `stdlib.h`.
- **🛠️ System Calls**: Basic infrastructure for user-space programs to interact with the kernel (via `int 0x80`).

---

## 🏗️ Project Structure

```text
.
├── kernel/                 # Kernel source code
│   ├── arch/i386/          # x86-specific architecture code (GDT, IDT, Paging)
│   └── include/            # Kernel-wide headers
├── libc/                   # Minimal C library implementation
├── docs/                   # Detailed technical documentation & guides
├── cmake/                  # Build system configuration
├── scripts/                # Utility scripts for ISO generation
├── isodir/                 # Temporary directory for GRUB ISO creation
└── Makefile                # Convenient wrapper for build commands
```

---

## 📚 Documentation

Comprehensive technical documentation, including architecture deep-dives and development guides, is available at:
👉 **[sohamtilekar.github.io/TilekarOS/](https://sohamtilekar.github.io/TilekarOS/)**

---

## 🚀 Quick Start

### 🧰 Prerequisites

Ensure you have the following tools installed:

- **Compiler**: `Clang` (Cross-compiling for `i386-elf`)
- **Assembler**: `NASM`
- **Build System**: `CMake` & `Make`
- **Emulator**: `QEMU` (specifically `qemu-system-i386`)
- **Bootloader Tools**: `grub-common`, `mtools`, `xorriso` (for `grub-mkrescue`)

### 🛠️ Build and Run

TilekarOS uses a `Makefile` wrapper around **CMake** for a seamless development experience.

```bash
# Clone the repository
git clone https://github.com/sohamtilekar/TilekarOS.git
cd TilekarOS

# Configure, build the ISO, and launch in QEMU
make run_iso

# To build only the kernel binary (without ISO)
make

# To clean build artifacts
make clean
```

---

## 🤝 Contributing

Contributions are welcome! Whether it's fixing a bug, improving documentation, or adding a new driver, feel free to open an issue or submit a pull request.

1. Fork the Project
2. Create your Feature Branch (`git checkout -b feature/AmazingFeature`)
3. Commit your Changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the Branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

---

## 📜 License

Distributed under the **MIT License**. See `LICENSE` for more information.

---

## 🍎 Acknowledgments

- [OSDev Wiki](https://wiki.osdev.org/) - The ultimate resource for OS developers.
- [Philipp Oppermann's Writing an OS in Rust](https://os.phil-opp.com/) - For architectural inspiration.
- All the hobbyist OS developers sharing their knowledge online!

