# Installation & Setup

To develop TilekarOS, you need a cross-compilation environment for i386.

## 1. Prerequisites (Ubuntu/Debian)

Install the necessary build tools and emulators:

```bash
sudo apt update
sudo apt install build-essential cmake nasm qemu-system-x86 grub-pc-bin xorriso mtools python3 clang lld
```

## 2. Recommended IDE

We recommend using **VS Code** or **Zed** with the following extensions:
- **Clangd**: For C/C++ intellisense and cross-compilation support.
- **ASM-LSP**: For NASM syntax highlighting and completions.

The project root contains a `compile_flags.txt` and `.clangd` file which are automatically generated to help your editor understand the kernel headers.

## 3. Cloning the Repository

```bash
git clone https://github.com/your-repo/TilekarOS.git
cd TilekarOS
```

## 4. First Build

```bash
make configure
make iso
make run_iso
```
