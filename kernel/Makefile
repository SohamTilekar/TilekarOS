CLANG_TARGET = i386-elf
NASM_FORMAT = elf32
ARCH = i386

C_SOURCES := $(wildcard *.c arch/$(ARCH)/*.c)
C_OBJECTS := $(C_SOURCES:%.c=../dump/%.c.o)
ASM_SOURCES := $(wildcard arch/$(ARCH)/*.asm)
ASM_OBJECTS := $(ASM_SOURCES:%.asm=../dump/%.asm.o)
LOCAL_CONFIG_SOURCE := ./arch/$(ARCH)/local_config.h
CONFIG_SOURCE := ./include/kernel/config.h
NASM_LOCAL_CONFIG_INC := ../dump/local_config.inc
NASM_CONFIG_INC := ../dump/config.inc

KERNEL_BIN := ../dump/myos.bin
LIBC_OBJ := ../sysroot/usr/obj/libc.o

# Debug variables
DUMP_DEBUG_DIR := ../dump_debug
C_OBJECTS_DEBUG := $(C_SOURCES:%.c=$(DUMP_DEBUG_DIR)/%.c.o)
ASM_OBJECTS_DEBUG := $(ASM_SOURCES:%.asm=$(DUMP_DEBUG_DIR)/%.asm.o)
KERNEL_BIN_DEBUG := $(DUMP_DEBUG_DIR)/myos_debug.bin

# Default target
all: $(KERNEL_BIN)

# Sysroot setup (only runs when missing headers)
sysrootsetup:
	mkdir -p ../sysroot/usr/include/
	cp -ru ./include/* ../sysroot/usr/include/

$(NASM_LOCAL_CONFIG_INC): $(LOCAL_CONFIG_SOURCE) ../helpers/h2inc.py
	mkdir -p $(dir $@)
	python3 ../helpers/h2inc.py $< $@

$(NASM_CONFIG_INC): $(CONFIG_SOURCE) ../helpers/h2inc.py
	mkdir -p $(dir $@)
	python3 ../helpers/h2inc.py $< $@

# Compile C files to object files
../dump/%.c.o: %.c | sysrootsetup
	mkdir -p $(dir $@)
	clang --target=$(CLANG_TARGET) --sysroot=../sysroot -I ./include -c $< -o $@ -nostdlib -ffreestanding -O2 -Wall -Wextra

# Compile C files to object files with debug info
$(DUMP_DEBUG_DIR)/%.c.o: %.c | sysrootsetup
	mkdir -p $(dir $@)
	clang --target=$(CLANG_TARGET) --sysroot=../sysroot -I ./include -c $< -o $@ -nostdlib -ffreestanding -O2 -Wall -Wextra -g

# Assemble .asm files to object files
../dump/%.asm.o: %.asm $(NASM_LOCAL_CONFIG_INC) $(NASM_CONFIG_INC)
	mkdir -p $(dir $@)
	nasm -f$(NASM_FORMAT) $< -o $@ -P $(NASM_LOCAL_CONFIG_INC) -P $(NASM_CONFIG_INC)

# Assemble .asm files to object files with debug info
$(DUMP_DEBUG_DIR)/%.asm.o: %.asm $(NASM_LOCAL_CONFIG_INC) $(NASM_CONFIG_INC)
	mkdir -p $(dir $@)
	nasm -f$(NASM_FORMAT) $< -o $@ -g -F dwarf -P $(NASM_LOCAL_CONFIG_INC) -P $(NASM_CONFIG_INC)

# Build libc only if needed
$(LIBC_OBJ):
	cd ../libc && make build

# Link everything into the kernel binary
$(KERNEL_BIN): $(ASM_OBJECTS) $(C_OBJECTS) $(LIBC_OBJ)
	clang --target=$(CLANG_TARGET) --sysroot=../sysroot -T arch/$(ARCH)/linker.ld \
	-nostdlib -ffreestanding -O2 -Wall -Wextra $(ASM_OBJECTS) $(C_OBJECTS) $(LIBC_OBJ) -o $@

# Link everything into the debug kernel binary
$(KERNEL_BIN_DEBUG): $(ASM_OBJECTS_DEBUG) $(C_OBJECTS_DEBUG) $(LIBC_OBJ)
	clang --target=$(CLANG_TARGET) --sysroot=../sysroot -T arch/$(ARCH)/linker.ld \
	-nostdlib -ffreestanding -O2 -Wall -Wextra $(ASM_OBJECTS_DEBUG) $(C_OBJECTS_DEBUG) $(LIBC_OBJ) -o $@ -g

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

# ISO debug build
iso_debug: $(KERNEL_BIN_DEBUG)
	mkdir -p $(DUMP_DEBUG_DIR)/isodir/boot/grub
	cp $(KERNEL_BIN_DEBUG) $(DUMP_DEBUG_DIR)/isodir/boot/myos.kernel
	cp -u ../grub.cfg $(DUMP_DEBUG_DIR)/isodir/boot/grub/grub.cfg
	grub-mkrescue -o $(DUMP_DEBUG_DIR)/myos.iso $(DUMP_DEBUG_DIR)/isodir

# Run debug targets
rund: $(KERNEL_BIN_DEBUG)
	qemu-system-$(ARCH) -kernel $(KERNEL_BIN_DEBUG) -s -S

run_isod: iso_debug
	qemu-system-$(ARCH) -cdrom $(DUMP_DEBUG_DIR)/myos.iso -s -S

clean:
	rm -r ../dump/ $(DUMP_DEBUG_DIR)
