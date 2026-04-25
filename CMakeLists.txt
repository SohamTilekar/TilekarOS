cmake_minimum_required(VERSION 3.15)

# Set the default architecture to i386 if not specified.
if(NOT DEFINED OS_ARCH)
    set(OS_ARCH "i386" CACHE STRING "Target architecture" FORCE)
endif()

# Build NASM objects as 32-bit by default for TilekarOS (i386).
if(OS_ARCH STREQUAL "i386")
    set(CMAKE_ASM_NASM_OBJECT_FORMAT elf32)
endif()

# Keep toolchain behavior deterministic across distros.
if(NOT CMAKE_C_COMPILER_TARGET)
    set(CMAKE_C_COMPILER_TARGET "${OS_ARCH}-elf")
endif()

if(NOT CMAKE_TRY_COMPILE_TARGET_TYPE)
    set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
endif()

find_program(TILEKAR_LD NAMES ld.bfd ld)
find_program(TILEKAR_AR NAMES ar)
find_program(TILEKAR_NM NAMES nm)
find_program(TILEKAR_RANLIB NAMES ranlib)
find_program(TILEKAR_OBJCOPY NAMES objcopy)
find_program(TILEKAR_READELF NAMES readelf)
find_program(TILEKAR_STRIP NAMES strip)

if(TILEKAR_LD)
    set(CMAKE_LINKER "${TILEKAR_LD}" CACHE FILEPATH "GNU linker" FORCE)
endif()
if(TILEKAR_AR)
    set(CMAKE_AR "${TILEKAR_AR}" CACHE FILEPATH "Archive tool" FORCE)
    set(CMAKE_C_COMPILER_AR "${TILEKAR_AR}" CACHE FILEPATH "C compiler archive tool" FORCE)
endif()
if(TILEKAR_NM)
    set(CMAKE_NM "${TILEKAR_NM}" CACHE FILEPATH "Symbol tool" FORCE)
endif()
if(TILEKAR_RANLIB)
    set(CMAKE_RANLIB "${TILEKAR_RANLIB}" CACHE FILEPATH "Index archive tool" FORCE)
    set(CMAKE_C_COMPILER_RANLIB "${TILEKAR_RANLIB}" CACHE FILEPATH "C compiler ranlib tool" FORCE)
endif()
if(TILEKAR_OBJCOPY)
    set(CMAKE_OBJCOPY "${TILEKAR_OBJCOPY}" CACHE FILEPATH "Binary copy tool" FORCE)
endif()
if(TILEKAR_READELF)
    set(CMAKE_READELF "${TILEKAR_READELF}" CACHE FILEPATH "ELF inspection tool" FORCE)
endif()
if(TILEKAR_STRIP)
    set(CMAKE_STRIP "${TILEKAR_STRIP}" CACHE FILEPATH "Binary strip tool" FORCE)
endif()

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
set(CLANGD_CONTENT "${CLANGD_CONTENT}    - -fno-stack-protector\n")
set(CLANGD_CONTENT "${CLANGD_CONTENT}    - -mno-sse\n")
set(CLANGD_CONTENT "${CLANGD_CONTENT}    - -mno-mmx\n")
set(CLANGD_CONTENT "${CLANGD_CONTENT}    - -mno-sse2\n")

# Add include paths for the kernel and libc
set(CLANGD_CONTENT "${CLANGD_CONTENT}    - -Ikernel/include\n")
set(CLANGD_CONTENT "${CLANGD_CONTENT}    - -Ilibc/include\n")
set(CLANGD_CONTENT "${CLANGD_CONTENT}    - -Iuserland/shell\n")
# Add include path for the sysroot (simulating where installed headers live)
set(CLANGD_CONTENT "${CLANGD_CONTENT}    - -Isysroot/usr/include\n")

# Write the configuration to the project root
file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/.clangd" "${CLANGD_CONTENT}")
