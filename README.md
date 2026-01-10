# TilekarOS

TilekarOS is a hobby 32-bit operating system written in C and Assembly for the x86 architecture.

## 📚 Documentation

Detailed documentation is available in the [`docs/`](./docs/) directory:

*   **[Introduction & Roadmap](./docs/README.md)**
*   **[Architecture & Internals](./docs/ARCHITECTURE.md)**
*   **[Build System](./docs/BUILD_SYSTEM.md)**

## 🚀 Quick Start

To build and run the OS (requires QEMU, Clang, NASM, and GRUB):

```bash
# Build and run the ISO
make run_iso

# Build only the kernel binary
make

# Debug mode (wait for GDB)
make rund
```
