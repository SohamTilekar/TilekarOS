CLANG_TARGET = i386-elf
NASM_FORMAT = elf32
ARCH = i386

C_SOURCES := $(wildcard *.c arch/$(ARCH)/*.c)
C_OBJECTS := $(C_SOURCES:%.c=../dump/%.o)
ASM_SOURCES := $(wildcard arch/$(ARCH)/*.asm)
ASM_OBJECTS := $(ASM_SOURCES:%.asm=../dump/%.o)

KERNEL_BIN := ../dump/myos.bin
LIBC_OBJ := ../sysroot/usr/obj/libc.o

# Default target
all: $(KERNEL_BIN)

# Sysroot setup (only runs when missing headers)
sysrootsetup:
	mkdir -p ../sysroot/usr/include/
	cp -ru ./include/* ../sysroot/usr/include/

# Compile C files to object files
../dump/%.o: %.c | sysrootsetup
	mkdir -p $(dir $@)
	clang --target=$(CLANG_TARGET) --sysroot=../sysroot -I ./include -c $< -o $@ -nostdlib -ffreestanding -O2 -Wall -Wextra

# Assemble .asm files to object files
../dump/%.o: %.asm
	mkdir -p $(dir $@)
	nasm -f$(NASM_FORMAT) $< -o $@

# Build libc only if needed
$(LIBC_OBJ):
	cd ../libc && make build

# Link everything into the kernel binary
$(KERNEL_BIN): $(ASM_OBJECTS) $(C_OBJECTS) $(LIBC_OBJ)
	clang --target=$(CLANG_TARGET) --sysroot=../sysroot -T arch/$(ARCH)/linker.ld \
	-nostdlib -ffreestanding -O2 -Wall -Wextra $(ASM_OBJECTS) $(C_OBJECTS) $(LIBC_OBJ) -o $@

# ISO build
iso: $(KERNEL_BIN)
	mkdir -p ../isodir/boot/grub
	cp $(KERNEL_BIN) ../isodir/boot/myos.kernel
	cp -u ../grub.cfg ../isodir/boot/grub/grub.cfg
	grub-mkrescue -o ../dump/myos.iso ../isodir

# Run targets
run: $(KERNEL_BIN)
	qemu-system-$(ARCH) -kernel $(KERNEL_BIN)

run_iso: iso
	qemu-system-$(ARCH) -cdrom ../dump/myos.iso

clean:
	rm -rf ../dump/*.o $(KERNEL_BIN) ../dump/myos.iso
