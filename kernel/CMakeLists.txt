
# ==============================================================================
# External Dependencies
# ==============================================================================
# We need Python 3 to run the 'h2inc.py' helper script, which syncs C constants
# to Assembly.
find_package(Python3 REQUIRED)

# ==============================================================================
# Source File Discovery
# ==============================================================================
# Gather all C and Assembly source files.
file(GLOB KERNEL_C_SOURCES "*.c")
file(GLOB KERNEL_ASM_SOURCES "*.asm")
file(GLOB_RECURSE ARCH_C_SOURCES "arch/${OS_ARCH}/*.c")
file(GLOB_RECURSE ARCH_ASM_SOURCES "arch/${OS_ARCH}/*.asm")

set(ALL_SOURCES
    ${KERNEL_C_SOURCES}
    ${KERNEL_ASM_SOURCES}
    ${ARCH_C_SOURCES}
    ${ARCH_ASM_SOURCES}
)

# ==============================================================================
# C Header -> NASM Include Generation
# ==============================================================================
# We define system constants (like GDT offsets) in C headers (.h).
# The Assembly code (.asm) needs these exact same values.
# 'h2inc.py' reads the C header and generates a NASM-compatible .inc file.

set(CONFIG_H "${CMAKE_CURRENT_SOURCE_DIR}/include/kernel/config.h")
set(LOCAL_CONFIG_H "${CMAKE_CURRENT_SOURCE_DIR}/arch/${OS_ARCH}/cpu/local_config.h")

# The generated files are placed in the build directory (BINARY_DIR), not source.
set(CONFIG_INC "${CMAKE_CURRENT_BINARY_DIR}/config.inc")
set(LOCAL_CONFIG_INC "${CMAKE_CURRENT_BINARY_DIR}/local_config.inc")

# Rule to generate config.inc
# Command: python3 helpers/h2inc.py include/kernel/config.h build/kernel/config.inc
add_custom_command(
    OUTPUT ${CONFIG_INC}
    COMMAND Python3::Interpreter "${CMAKE_SOURCE_DIR}/helpers/h2inc.py" ${CONFIG_H} ${CONFIG_INC}
    DEPENDS ${CONFIG_H} "${CMAKE_SOURCE_DIR}/helpers/h2inc.py"
    COMMENT "Generating config.inc from config.h"
)

# Rule to generate local_config.inc
# Command: python3 helpers/h2inc.py arch/i386/cpu/local_config.h build/kernel/local_config.inc
add_custom_command(
    OUTPUT ${LOCAL_CONFIG_INC}
    COMMAND Python3::Interpreter "${CMAKE_SOURCE_DIR}/helpers/h2inc.py" ${LOCAL_CONFIG_H} ${LOCAL_CONFIG_INC}
    DEPENDS ${LOCAL_CONFIG_H} "${CMAKE_SOURCE_DIR}/helpers/h2inc.py"
    COMMENT "Generating local_config.inc from local_config.h"
)

# Create a 'custom target' that depends on the generated files.
# This ensures they are built before any target that depends on 'kernel_asm_headers'.
add_custom_target(kernel_asm_headers DEPENDS ${CONFIG_INC} ${LOCAL_CONFIG_INC})

# ==============================================================================
# Kernel Executable Target
# ==============================================================================
# We enable NASM support explicitly here.
enable_language(ASM_NASM)

# Define the kernel executable target.
add_executable(myos.kernel ${ALL_SOURCES})

# Ensure the .inc files are generated BEFORE we try to compile the .asm files.
add_dependencies(myos.kernel kernel_asm_headers)

# ==============================================================================
# Test Flag Configuration
# ==============================================================================
# ENABLE_TEST: If 1, automatically run tests without user prompt; if 0, skip tests
if(NOT DEFINED ENABLE_TEST)
    set(ENABLE_TEST 0)
endif()
message(STATUS "ENABLE_TEST: ${ENABLE_TEST}")

# ==============================================================================
# Compile Options
# ==============================================================================
set(KERNEL_NASM_FORMAT_FLAG "")
if(OS_ARCH STREQUAL "i386")
    # Without the toolchain file, NASM may default to 64-bit objects on x86_64 hosts.
    # Force elf32 so 32-bit boot/kernel assembly always builds correctly.
    set(KERNEL_NASM_FORMAT_FLAG "-felf32")
endif()

target_compile_options(myos.kernel PRIVATE
    # C Compiler Flags
    $<$<COMPILE_LANGUAGE:C>:
        -nostdlib          # Do not link standard system libraries
        -ffreestanding     # Target is freestanding (no OS support assumed)
        -fno-pic           # No Position Independent Code (Kernel relies on absolute addresses)
        -fno-pie           # No Position Independent Executable
        -fno-stack-protector # Disable stack canaries
        -fno-builtin       # Prevent replacing loops with implicitly assumed stdlib functions
        -mno-sse           # Disable vector instructions
        -mno-mmx           # Disable MMX instructions
        -mno-sse2          # Disable SSE2 instructions
        -O0                # Optimization level 0 (easier debugging)
        -Wall              # Enable all warnings
        -Wextra            # Enable extra warnings
        -g                 # Generate debug information
        --target=${CMAKE_C_COMPILER_TARGET} # Target architecture from toolchain
        --sysroot=${CMAKE_SOURCE_DIR}/sysroot # Use our custom sysroot
        -DENABLE_TEST=${ENABLE_TEST}
    >    # Assembler Flags (NASM)
    $<$<COMPILE_LANGUAGE:ASM_NASM>:
        ${KERNEL_NASM_FORMAT_FLAG}
        ${CMAKE_ASM_NASM_FLAGS}
        -g                 # Generate debug info
        -F dwarf           # Debug format: DWARF
        # Pre-include the generated config files so they are available globally in asm
        -P${LOCAL_CONFIG_INC}
        -P${CONFIG_INC}
    >
)

# ------------------------------------------------------------------------------
# Include Directories
# ------------------------------------------------------------------------------
target_include_directories(myos.kernel PRIVATE
    include                         # Kernel internal headers
    ${CMAKE_SOURCE_DIR}/libc/include # Standard library headers
    arch/${OS_ARCH}
    arch/${OS_ARCH}/cpu
    arch/${OS_ARCH}/drivers
    arch/${OS_ARCH}/fs
    arch/${OS_ARCH}/mm
    arch/${OS_ARCH}/task
    arch/${OS_ARCH}/syscalls
    arch/${OS_ARCH}/utils
)

# ------------------------------------------------------------------------------
# Linker Options
# ------------------------------------------------------------------------------
set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/arch/${OS_ARCH}/boot/linker.ld")

target_link_options(myos.kernel PRIVATE
    -T ${LINKER_SCRIPT}     # Use our custom linker script
    -nostdlib               # No standard startup files
    -static                 # Static linking only
    -ffreestanding
    -fuse-ld=bfd            # Force GNU BFD linker for stable bare-metal layout
    -no-pie                 # Prevent Position Independent Executable linking
    -Wl,-m,elf_i386         # Force i386 ELF emulation mode in GNU ld
    -Wl,-z,max-page-size=0x1000 # Fix section alignment for newer binutils
    -Wl,-z,common-page-size=0x1000 # Keep common page size at 4KiB as well
    -Wl,--build-id=none     # Disable build-id generation to keep binary clean
    --target=${CMAKE_C_COMPILER_TARGET}
    --sysroot=${CMAKE_SOURCE_DIR}/sysroot
)

# Link the kernel with our custom libc.
target_link_libraries(myos.kernel PRIVATE libc)

# ==============================================================================
# ISO Image Generation
# ==============================================================================
# This section handles packaging the kernel binary into a bootable ISO using GRUB.

set(ISODIR "${CMAKE_BINARY_DIR}/isodir")
set(ISO_IMAGE "${CMAKE_BINARY_DIR}/myos.iso")
set(GRUB_CFG "${CMAKE_SOURCE_DIR}/grub.cfg")

# Rule to generate the bootable ISO image
# Commands:
#   mkdir -p build/isodir/boot/grub
#   cp build/kernel/myos.kernel build/isodir/boot/myos.kernel
#   cp grub.cfg build/isodir/boot/grub/grub.cfg
#   grub-mkrescue -o build/myos.iso build/isodir
add_custom_command(
    OUTPUT ${ISO_IMAGE}
    # Create the directory structure: isodir/boot/grub
    COMMAND ${CMAKE_COMMAND} -E make_directory ${ISODIR}/boot/grub
    # Copy the sysroot content to isodir root
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/sysroot/ ${ISODIR}/
    # Copy the kernel binary
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:myos.kernel> ${ISODIR}/boot/myos.kernel
    # Copy the GRUB configuration
    COMMAND ${CMAKE_COMMAND} -E copy ${GRUB_CFG} ${ISODIR}/boot/grub/grub.cfg
    # Generate the ISO using grub-mkrescue
    COMMAND grub-mkrescue -o ${ISO_IMAGE} ${ISODIR}
    # Re-run this if kernel or grub.cfg changes
    DEPENDS myos.kernel ${GRUB_CFG}
    COMMENT "Generating ISO image: ${ISO_IMAGE}"
)

# 'make iso' target
add_custom_target(iso DEPENDS ${ISO_IMAGE})

# ==============================================================================
# Runtime targets
# ==============================================================================
# Runtime and VM orchestration are handled by helpers/build.py (invoked via Makefile),
# so CMake only keeps build artifact targets.
