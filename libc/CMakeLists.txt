
# ==============================================================================
# LibC Source Discovery
# ==============================================================================
# We separate architecture-independent (generic) sources from architecture-specific ones.

# 1. Generic Sources (stdio, stdlib, string, etc.)
# We explicitly list subdirectories to avoid accidentally including 'arch' via GLOB_RECURSE.
file(GLOB LIBC_STDIO_SOURCES CONFIGURE_DEPENDS "stdio/*.c")
file(GLOB LIBC_STDLIB_SOURCES CONFIGURE_DEPENDS "stdlib/*.c")
file(GLOB LIBC_STRING_SOURCES CONFIGURE_DEPENDS "string/*.c")
file(GLOB LIBC_CTYPE_SOURCES CONFIGURE_DEPENDS "ctype/*.c")
file(GLOB LIBC_ERRNO_SOURCES CONFIGURE_DEPENDS "errno/*.c")
file(GLOB LIBC_SIGNAL_SOURCES CONFIGURE_DEPENDS "signal/*.c")

# 2. Architecture Specific Sources
# Only include the sources for the currently selected OS_ARCH.
file(GLOB LIBC_ARCH_SOURCES CONFIGURE_DEPENDS "arch/${OS_ARCH}/*.c")

set(LIBC_SOURCES
    ${LIBC_STDIO_SOURCES}
    ${LIBC_STDLIB_SOURCES}
    ${LIBC_STRING_SOURCES}
    ${LIBC_CTYPE_SOURCES}
    ${LIBC_ERRNO_SOURCES}
    ${LIBC_SIGNAL_SOURCES}
    ${LIBC_ARCH_SOURCES}
)

# ==============================================================================
# LibC Target Definition
# ==============================================================================
# Create a STATIC library named 'libc'.
add_library(libc STATIC ${LIBC_SOURCES})

# ------------------------------------------------------------------------------
# Compile Options
# ------------------------------------------------------------------------------
target_compile_options(libc PRIVATE
    -nostdlib        # No standard system libraries
    -ffreestanding   # Freestanding environment (no OS)
    -O2              # Optimize for speed (standard for libs)
    -Wall            # Enable warnings
    -Wextra          # Enable extra warnings
    --target=${CMAKE_C_COMPILER_TARGET} # Target architecture from toolchain
)

# ------------------------------------------------------------------------------
# Include Directories
# ------------------------------------------------------------------------------
target_include_directories(libc PRIVATE 
    include
    ${CMAKE_SOURCE_DIR}/kernel/include
)
target_include_directories(libc INTERFACE include)

# ==============================================================================
# Sysroot Population
# ==============================================================================
set(SYSROOT_DIR "${CMAKE_SOURCE_DIR}/sysroot")
set(SYSROOT_INCLUDE_DIR "${SYSROOT_DIR}/usr/include")
set(SYSROOT_LIB_DIR "${SYSROOT_DIR}/usr/lib")

# 1. Headers (LibC + Kernel)
add_custom_command(
    TARGET libc POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${SYSROOT_INCLUDE_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/include ${SYSROOT_INCLUDE_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/kernel/include ${SYSROOT_INCLUDE_DIR}
    COMMENT "Populating sysroot with headers..."
)

# 2. Static Library
add_custom_command(
    TARGET libc POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${SYSROOT_LIB_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:libc> ${SYSROOT_LIB_DIR}/libc.a
    COMMENT "Copying libc to sysroot..."
)

# 3. Startup Code (crt0) and Linker Script
set(CRT0_SRC "${CMAKE_CURRENT_SOURCE_DIR}/arch/${OS_ARCH}/crt0.asm")
set(CRT0_OBJ "${SYSROOT_LIB_DIR}/crt0.o")
set(LINKER_SRC "${CMAKE_CURRENT_SOURCE_DIR}/arch/${OS_ARCH}/user.ld")

add_custom_command(
    OUTPUT ${CRT0_OBJ}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${SYSROOT_LIB_DIR}
    COMMAND nasm -f elf32 ${CRT0_SRC} -o ${CRT0_OBJ}
    DEPENDS ${CRT0_SRC}
    COMMENT "Assembling crt0 for sysroot..."
)

set(SYSROOT_LINKER "${SYSROOT_LIB_DIR}/user.ld")

add_custom_command(
    OUTPUT ${SYSROOT_LINKER}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${SYSROOT_LIB_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy ${LINKER_SRC} ${SYSROOT_LINKER}
    DEPENDS ${LINKER_SRC}
    COMMENT "Copying userspace linker script to sysroot..."
)

# Create a target for sysroot components that aren't the main library
add_custom_target(sysroot_extras ALL DEPENDS ${CRT0_OBJ} ${SYSROOT_LINKER})
add_dependencies(sysroot_extras libc)
