# ELF User Task Integration Guide

This guide explains how to compile a test user process from assembly to an ELF file, embed it into the kernel, and load it using the kernel's ELF task loader.
## 1. Create the User Task Assembly (`user_task.asm.ignore`)

Define your user process in `kernel/user_task.asm.ignore`. For example:

```asm
[bits 32]
section .text
global _start

_start:
    ; A simple infinite loop for the test user process
    mov eax, 0xCAFEBABE
.loop:
    jmp .loop
```

We add .ignore at the end so that it wont conflict with the kernel iso

## 2. Compile and Link to ELF

Use the following commands to compile the NASM code to an object file and then link it into a standalone ELF binary for the i386 target:

```bash
nasm -f elf32 kernel/user_task.asm.ignore -o kernel/user_task.o
clang -target i386-pc-none-elf -march=i386 -nostdlib -static -Wl,-e,_start kernel/user_task.o -o kernel/user_task.elf
```

- `nasm -f elf32`: Compiles the NASM file into a 32-bit ELF object file.
- `clang -target i386-pc-none-elf`: Links the object file into a standalone ELF binary.
- `-nostdlib`: Prevents linking against standard libraries.
- `-static`: Creates a static binary.
- `-Wl,-e,_start`: Sets the entry point to `_start`.


## 3. Embed the ELF in the Kernel (`user_process.asm`)

Create a bridge assembly file to include the ELF binary. Use an **absolute path** for the `incbin` directive to ensure NASM can locate the file regardless of the build directory or missing include paths in `CMakeLists.txt`:

```asm
[bits 32]
section .rodata

global _start_elf_user_task
global _end_elf_user_task

_start_elf_user_task:
    incbin "/mnt/soham/soham_code/TilekarOS/kernel/user_task.elf"
_end_elf_user_task:
```

> **Note:** If you move the project directory, you must update this path.

## 4. Load the ELF Task in the Kernel (`kernel.c`)

Declare the symbols and use `task_create_elf`:

```c
extern char _start_elf_user_task;
extern char _end_elf_user_task;

// Inside kernel_main
uint32_t elf_size = (uint32_t)(&_end_elf_user_task - &_start_elf_user_task);
printf("Creating ELF task from user_task.elf (size: %u bytes)...\n", elf_size);
task_create_elf(&_start_elf_user_task, elf_size);
```

## 5. Important Note

Do NOT add these new files to `CMakeLists.txt` as per current requirements. They are for manual testing and demonstration.
