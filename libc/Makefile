CLANG_TARGET = i386-elf
NASM_FORMAT = elf32
ARCH = i386

SRC := $(wildcard */*.c)
OBJ := $(patsubst %.c, ../sysroot/usr/lib/%.o, $(SRC))
LIB := ../sysroot/usr/obj/libc.o

# Main build target (used by kernel Makefile)
build: $(LIB)

# Sysroot setup — only runs if directories/headers missing
sysrootsetup:
	mkdir -p ../sysroot/usr/include
	mkdir -p ../sysroot/usr/lib
	mkdir -p ../sysroot/usr/obj
	cp -ru ./include/* ../sysroot/usr/include/

# Combine all object files into a single relocatable object
$(LIB): $(OBJ)
	ld.lld -r -o $@ $(OBJ)

# Compile .c files to .o, only when sources or headers change
../sysroot/usr/lib/%.o: %.c | sysrootsetup
	mkdir -p $(dir $@)
	clang --target=$(CLANG_TARGET) -I ./include -nostdlib -ffreestanding -O2 -Wall -Wextra -c $< -o $@

clean:
	rm -f $(OBJ) $(LIB)
