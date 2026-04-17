# Installation & Setup

To develop TilekarOS, you need a cross-compilation environment for i386.

## 1. Prerequisites

### Ubuntu / Debian
```bash
sudo apt update
sudo apt install build-essential cmake nasm qemu-system-x86 grub-pc-bin xorriso mtools python3 clang lld
```

### Fedora
```bash
sudo dnf groupinstall "Development Tools"
sudo dnf install cmake nasm qemu-system-x86 grub2-tools-extra xorriso mtools python3 clang lld
```

### macOS (Homebrew)
On macOS, you'll need `i686-elf` binutils and potentially a cross-compiler if you aren't using the provided Clang setup.
```bash
brew install i686-elf-binutils cmake nasm qemu xorriso mtools python3 llvm
```
*Note: Ensure `llvm` is in your PATH to use `clang` and `lld` for cross-compilation.*

---

## 2. Recommended IDE

We recommend using **VS Code** or **Zed** with the following extensions:
- **Clangd**: For C/C++ intellisense and cross-compilation support.
- **ASM-LSP**: For NASM syntax highlighting and completions.

The project root contains a `compile_flags.txt` and `.clangd` file which are automatically generated to help your editor understand the kernel headers.

**Source Files**: [.clangd](https://github.com/SohamTilekar/TilekarOS/blob/main/.clangd){: target="_blank" }, [compile_flags.txt](https://github.com/SohamTilekar/TilekarOS/blob/main/compile_flags.txt){: target="_blank" }

??? example "Code Preview: `.clangd`"
    ```yaml
    <--8<-- ".clangd"
    ```

---

## 3. Common Pitfalls

- **`grub-mkrescue` issues**: On some systems, `grub-mkrescue` requires `grub-pc-bin` (Ubuntu) or `grub2-tools-extra` (Fedora). Without these, ISO creation will fail.
- **`mtools` missing**: `mtools` is required for FAT12 filesystem manipulation during the build process.
- **Case-Insensitive FS**: If building on macOS or Windows (via WSL), ensure your filesystem is case-sensitive or be careful with filename casing in includes.

---

## 4. Cloning the Repository

```bash
git clone https://github.com/SohamTilekar/TilekarOS.git
cd TilekarOS
```

## 5. First Build

```bash
make configure
make iso
make run_iso
```

---

## 6. Test/Example: Verifying your Toolchain

Run the following command to ensure your compiler is targeting the correct architecture:

```bash
clang --target=i386-elf -v
```

You should see output indicating `Target: i386-unknown-elf`.

---

## References
- [OSDev: Getting Started](https://wiki.osdev.org/Getting_Started)
- [LLVM/Clang Documentation](https://clang.llvm.org/docs/)
- [QEMU Documentation](https://www.qemu.org/docs/master/)
