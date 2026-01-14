
# ==============================================================================
# LibC Source Discovery
# ==============================================================================
# We separate architecture-independent (generic) sources from architecture-specific ones.

# 1. Generic Sources (stdio, stdlib, string, etc.)
# We explicitly list subdirectories to avoid accidentally including 'arch' via GLOB_RECURSE.
file(GLOB LIBC_STDIO_SOURCES "stdio/*.c")
file(GLOB LIBC_STDLIB_SOURCES "stdlib/*.c")
file(GLOB LIBC_STRING_SOURCES "string/*.c")

# 2. Architecture Specific Sources
# Only include the sources for the currently selected OS_ARCH.
file(GLOB LIBC_ARCH_SOURCES "arch/${OS_ARCH}/*.c")

set(LIBC_SOURCES
    ${LIBC_STDIO_SOURCES}
    ${LIBC_STDLIB_SOURCES}
    ${LIBC_STRING_SOURCES}
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
set(SYSROOT_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/sysroot/usr/include")

add_custom_command(
    TARGET libc PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${SYSROOT_INCLUDE_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/include ${SYSROOT_INCLUDE_DIR}
    COMMENT "Populating sysroot with libc headers..."
)
