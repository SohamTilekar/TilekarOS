cmake_minimum_required(VERSION 3.15)

# Build NASM objects as 32-bit by default for TilekarOS (i386).
set(CMAKE_ASM_NASM_OBJECT_FORMAT elf32)

# ==============================================================================
# Project: TilekarOS
# ==============================================================================
# Define the project name and enabled languages.
# C: For kernel and userspace code.
# ASM_NASM: For low-level x86 assembly (bootloader, interrupts, etc.).
# ==============================================================================
project(TilekarOS C ASM_NASM)

# ==============================================================================
# Architecture Configuration
# ==============================================================================
# Set the default architecture to i386 if not specified.
if(NOT DEFINED OS_ARCH)
    set(OS_ARCH "i386" CACHE STRING "Target architecture" FORCE)
endif()

if(NOT CMAKE_C_COMPILER_TARGET)
    set(CMAKE_C_COMPILER_TARGET "${OS_ARCH}-elf")
endif()

# Generate compile_commands.json for clangd and other tools
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

message(STATUS "Building for architecture: ${OS_ARCH}")

# ==============================================================================
# Toolchain Configuration
# ==============================================================================
# The toolchain file should ideally be passed via -DCMAKE_TOOLCHAIN_FILE=...
# However, if not provided, we try to guess based on OS_ARCH.
if(NOT CMAKE_TOOLCHAIN_FILE)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/toolchains/${OS_ARCH}.cmake")
        set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/toolchains/${OS_ARCH}.cmake")
    elseif(NOT CMAKE_C_COMPILER_TARGET)
        message(WARNING "No toolchain file found for architecture ${OS_ARCH}. Cross-compilation may fail.")
    endif()
endif()

# ==============================================================================
# Subdirectories
# ==============================================================================
# Add the 'libc' directory to the build. This will process libc/CMakeLists.txt
# and build the C standard library.
add_subdirectory(libc)

# Add the 'kernel' directory to the build. This will process kernel/CMakeLists.txt
# and build the kernel binary, linking it with libc.
add_subdirectory(kernel)

# ==============================================================================
# Custom Targets: Report
# ==============================================================================
# Add a target to print a detailed report of the build environment and tools.
# This helps in debugging issues across different machines.
# ==============================================================================
add_custom_target(report
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/helpers/build.py report
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Printing detailed system and tool report"
    VERBATIM
)

# ==============================================================================
# IDE Support: .clangd Generation
# ==============================================================================
# We generate a .clangd configuration file to help language servers (like clangd)
# understand our cross-compilation flags and include paths. This ensures that
# IntelliSense/Autocomplete works correctly in editors like VS Code or Zed.
# ==============================================================================

set(CLANGD_CONTENT "CompileFlags:\n  Add:\n")
# Tell clangd we are targeting the specific architecture
# We try to use the target set by the toolchain, or fallback to OS_ARCH-elf
if(CMAKE_C_COMPILER_TARGET)
    set(CLANGD_CONTENT "${CLANGD_CONTENT}    - --target=${CMAKE_C_COMPILER_TARGET}\n")
else()
    set(CLANGD_CONTENT "${CLANGD_CONTENT}    - --target=${OS_ARCH}-elf\n")
endif()

# Add critical flags for kernel development
set(CLANGD_CONTENT "${CLANGD_CONTENT}    - -ffreestanding\n")
set(CLANGD_CONTENT "${CLANGD_CONTENT}    - -nostdlib\n")
set(CLANGD_CONTENT "${CLANGD_CONTENT}    - -fno-builtin\n")
set(CLANGD_CONTENT "${CLANGD_CONTENT}    - -fno-pic\n")
set(CLANGD_CONTENT "${CLANGD_CONTENT}    - -fno-pie\n")

# Add include paths for the kernel and libc
set(CLANGD_CONTENT "${CLANGD_CONTENT}    - -Ikernel/include\n")
set(CLANGD_CONTENT "${CLANGD_CONTENT}    - -Ilibc/include\n")
set(CLANGD_CONTENT "${CLANGD_CONTENT}    - -Iuserland/shell\n")
# Add include path for the sysroot (simulating where installed headers live)
set(CLANGD_CONTENT "${CLANGD_CONTENT}    - -Isysroot/usr/include\n")

# Write the configuration to the project root
file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/.clangd" "${CLANGD_CONTENT}")
