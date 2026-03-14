# TilekarOS

TilekarOS is a hobby 32-bit operating system written in C and Assembly for the x86 architecture.

## 📚 Documentation

Detailed documentation is available in the [`docs/`](./docs/) directory:

*   **[Full Documentation Index](./docs/README.md)**
*   **[Architecture Overview](./docs/kernel/architecture.md)**
*   **[Build System](./docs/getting-started/build-system.md)**
*   **[Installation Guide](./docs/getting-started/installation.md)**

## 🚀 Quick Start

To build and run the OS, you need the following tools installed:

*   **CMake** (Build system generator)
*   **Clang** (Compiler)
*   **NASM** (Assembler)
*   **QEMU** (Emulator)
*   **GRUB** (Bootloader utilities, specifically `grub-mkrescue`)
*   **Xorriso** (Required by GRUB)

### Build & Run

We use a `Makefile` wrapper around **CMake** for convenience.

```bash
# Configure and build the ISO, then run it in QEMU
make run_iso

# Build only the kernel binary
make

# Clean build artifacts
make clean
```
