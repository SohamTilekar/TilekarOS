build:
	nasm -felf32 kernel/boot.asm -o dump/boot.o
	clang --target=i686-elf -c kernel/kernel.c -o dump/kernel.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
	clang --target=i686-elf -T kernel/linker.ld -nostdlib -ffreestanding -O2 dump/boot.o dump/kernel.o -o dump/myos.bin

run: build
	qemu-system-i386 -kernel ./dump/myos.bin
