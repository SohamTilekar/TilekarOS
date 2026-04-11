
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

# ------------------------------------------------------------------------------
# Compile Options
# ------------------------------------------------------------------------------
target_compile_options(myos.kernel PRIVATE
    # C Compiler Flags
    $<$<COMPILE_LANGUAGE:C>:
        -nostdlib          # Do not link standard system libraries
        -ffreestanding     # Target is freestanding (no OS support assumed)
        -fno-pic           # No Position Independent Code (Kernel relies on absolute addresses)
        -fno-pie           # No Position Independent Executable
        -O0                # Optimization level 0 (easier debugging)
        -Wall              # Enable all warnings
        -Wextra            # Enable extra warnings
        -g                 # Generate debug information
        --target=${CMAKE_C_COMPILER_TARGET} # Target architecture from toolchain
        --sysroot=${CMAKE_SOURCE_DIR}/sysroot # Use our custom sysroot
    >
    # Assembler Flags (NASM)
    $<$<COMPILE_LANGUAGE:ASM_NASM>:
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
# Run / Emulation Targets
# ==============================================================================

# Default VM directory if not provided
if(NOT VM_DIR)
    set(VM_DIR "VirtualMachine")
endif()

# Parse DRIVE_CONFIG (comma-separated list of NAME:SIZE[:INTERFACE]) to generate QEMU flags
set(QEMU_DRIVE_FLAGS "")
if(DRIVE_CONFIG)
    string(REPLACE "," ";" DRIVE_LIST ${DRIVE_CONFIG})
    set(IDE_NUM 0)
    set(AHCI_NUM 0)
    set(HAS_AHCI FALSE)
    foreach(ENTRY ${DRIVE_LIST})
        # Extract name and interface from ENTRY
        string(REPLACE ":" ";" PARTS ${ENTRY})
        list(GET PARTS 0 NAME)
        list(LENGTH PARTS LEN)
        
        # Default interface is 'ide'
        set(INTERFACE "ide")
        if(${LEN} GREATER 2)
            list(GET PARTS 2 INTERFACE)
        endif()

        if(${INTERFACE} STREQUAL "ahci")
            # AHCI Controller (ich9-ahci supports 6 ports)
            if(NOT HAS_AHCI)
                list(APPEND QEMU_DRIVE_FLAGS "-device" "ich9-ahci,id=ahci0")
                set(HAS_AHCI TRUE)
            endif()
            
            list(APPEND QEMU_DRIVE_FLAGS "-drive" "file=${CMAKE_SOURCE_DIR}/${VM_DIR}/drives/${NAME}.img,format=raw,if=none,id=ahci_d${AHCI_NUM}")
            list(APPEND QEMU_DRIVE_FLAGS "-device" "ide-hd,bus=ahci0.${AHCI_NUM},drive=ahci_d${AHCI_NUM}")
            math(EXPR AHCI_NUM "${AHCI_NUM} + 1")
        else()
            # Legacy IDE Logic
            math(EXPR BUS "${IDE_NUM} / 2")
            math(EXPR UNIT "${IDE_NUM} % 2")

            if(${IDE_NUM} LESS 4)
                # Use built-in IDE controller for first 4 drives
                list(APPEND QEMU_DRIVE_FLAGS "-drive" "file=${CMAKE_SOURCE_DIR}/${VM_DIR}/drives/${NAME}.img,format=raw,if=ide,bus=${BUS},unit=${UNIT}")
            else()
                # For 5th drive onwards, we need additional PCI IDE controllers.
                math(EXPR EXTRA_IDX "(${IDE_NUM} - 4) / 4 + 1")
                math(EXPR EXTRA_DRIVE_IDX "(${IDE_NUM} - 4) % 4")
                math(EXPR EXTRA_BUS "${EXTRA_DRIVE_IDX} / 2")
                math(EXPR EXTRA_UNIT "${EXTRA_DRIVE_IDX} % 2")

                # Add the controller if it's the first drive on it
                if(${EXTRA_DRIVE_IDX} EQUAL 0)
                    list(APPEND QEMU_DRIVE_FLAGS "-device" "piix4-ide,id=extide${EXTRA_IDX}")
                endif()

                list(APPEND QEMU_DRIVE_FLAGS "-drive" "file=${CMAKE_SOURCE_DIR}/${VM_DIR}/drives/${NAME}.img,format=raw,if=none,id=d${IDE_NUM}")
                list(APPEND QEMU_DRIVE_FLAGS "-device" "ide-hd,bus=extide${EXTRA_IDX}.${EXTRA_BUS},drive=d${IDE_NUM},unit=${EXTRA_UNIT}")
            endif()
            math(EXPR IDE_NUM "${IDE_NUM} + 1")
        endif()
    endforeach()
endif()

# QEMU Logging and Serial redirection to workspace folder
set(QEMU_LOG_FLAGS "-d" "guest_errors,unimp" "-D" "${CMAKE_SOURCE_DIR}/${VM_DIR}/qemu.log")
set(QEMU_SERIAL_FLAGS "-serial" "file:${CMAKE_SOURCE_DIR}/${VM_DIR}/serial.log")

# Create a display-friendly version of the drive flags for comments
string(REPLACE ";" " " QEMU_DRIVE_FLAGS_DISPLAY "${QEMU_DRIVE_FLAGS}")

# 'make run' - Runs the kernel binary directly (-kernel flag)
add_custom_target(run
    COMMAND qemu-system-${OS_ARCH} -kernel $<TARGET_FILE:myos.kernel> ${QEMU_DRIVE_FLAGS} ${QEMU_LOG_FLAGS} ${QEMU_SERIAL_FLAGS}
    DEPENDS myos.kernel
    COMMENT "Running: qemu-system-${OS_ARCH} -kernel $<TARGET_FILE:myos.kernel> ${QEMU_DRIVE_FLAGS_DISPLAY}"
)

# 'make run_iso' - Boots the ISO image (-cdrom flag)
add_custom_target(run_iso
    COMMAND qemu-system-${OS_ARCH} -cdrom ${ISO_IMAGE} ${QEMU_DRIVE_FLAGS} ${QEMU_LOG_FLAGS} ${QEMU_SERIAL_FLAGS}
    DEPENDS iso
    COMMENT "Running: qemu-system-${OS_ARCH} -cdrom ${ISO_IMAGE} ${QEMU_DRIVE_FLAGS_DISPLAY}"
)

# 'make run_headless' - Runs the kernel binary directly headless
add_custom_target(run_headless
    COMMAND /bin/sh -c "qemu-system-${OS_ARCH} -kernel $<TARGET_FILE:myos.kernel> -display none -serial file:${CMAKE_SOURCE_DIR}/${VM_DIR}/serial.log ${QEMU_DRIVE_FLAGS} ${QEMU_LOG_FLAGS} & sleep 2 && killall qemu-system-${OS_ARCH} || true"
    DEPENDS myos.kernel
    COMMENT "Running Headless: qemu-system-${OS_ARCH} -kernel $<TARGET_FILE:myos.kernel> -display none -serial file:${VM_DIR}/serial.log"
)

# 'make debug_run' - Runs the kernel binary directly with debug flags and connects GDB
add_custom_target(debug_run
    COMMAND /bin/sh -c "qemu-system-${OS_ARCH} -s -S -kernel $<TARGET_FILE:myos.kernel> ${QEMU_DRIVE_FLAGS} ${QEMU_LOG_FLAGS} ${QEMU_SERIAL_FLAGS} & gdb -ex 'target remote localhost:1234' -ex 'symbol-file $<TARGET_FILE:myos.kernel>'"
    DEPENDS myos.kernel
    COMMENT "Debugging: qemu-system-${OS_ARCH} -s -S -kernel $<TARGET_FILE:myos.kernel> & gdb ..."
    USES_TERMINAL
    VERBATIM
)

# 'make debug_iso' - Boots the ISO image with debug flags and connects GDB
add_custom_target(debug_iso
    COMMAND /bin/sh -c "qemu-system-${OS_ARCH} -s -S -cdrom ${ISO_IMAGE} ${QEMU_DRIVE_FLAGS} ${QEMU_LOG_FLAGS} ${QEMU_SERIAL_FLAGS} & gdb -ex 'target remote localhost:1234' -ex 'symbol-file $<TARGET_FILE:myos.kernel>'"
    DEPENDS iso
    COMMENT "Debugging ISO: qemu-system-${OS_ARCH} -s -S -cdrom ${ISO_IMAGE} & gdb ..."
    USES_TERMINAL
    VERBATIM
) # 'make run_bochs' - Runs the ISO in Bochs
add_custom_target(run_bochs
    COMMAND bochs -f bochs/bochsrc
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    DEPENDS iso
    COMMENT "Running: bochs -f bochs/bochsrc"
    USES_TERMINAL
    VERBATIM
)
