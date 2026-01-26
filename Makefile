# ==============================================================================
# Wrapper Makefile for TilekarOS
# ==============================================================================
# This Makefile acts as a convenient frontend for the CMake build system.
# Instead of typing long `cmake` commands, you can just type `make`, `make iso`, etc.
# ==============================================================================

# The directory where build artifacts will be generated.
BUILD_DIR = build

# Default Architecture
ARCH ?= i386

# Arguments passed to CMake configuration step.
# We pass the architecture variable and select the appropriate toolchain file.
CMAKE_ARGS = -DOS_ARCH=$(ARCH) -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/$(ARCH).cmake

.PHONY: all iso run run_iso run_bochs debug_run debug_iso clean configure help

# ------------------------------------------------------------------------------
# Targets
# ------------------------------------------------------------------------------

# 'make': Configures (if needed) and builds the project (default target).
all: configure
	cmake --build $(BUILD_DIR)

# 'make configure': Runs the CMake generation step.
# Creates the build directory and generates Makefiles/Ninja files inside it.
configure:
	cmake -B $(BUILD_DIR) $(CMAKE_ARGS)

# 'make iso': Builds the 'iso' target defined in kernel/CMakeLists.txt.
iso: configure
	cmake --build $(BUILD_DIR) --target iso

# 'make run': Builds the 'run' target (QEMU direct kernel boot).
run: configure
	cmake --build $(BUILD_DIR) --target run

# 'make run_iso': Builds the 'run_iso' target (QEMU ISO boot).
run_iso: configure
	cmake --build $(BUILD_DIR) --target run_iso

# 'make run_bochs': Builds the 'run_bochs' target (Bochs ISO boot).
run_bochs: configure
	cmake --build $(BUILD_DIR) --target run_bochs

# 'make debug_run': Builds the 'debug_run' target (QEMU direct debug).
debug_run: configure
	cmake --build $(BUILD_DIR) --target debug_run

# 'make debug_iso': Builds the 'debug_iso' target (QEMU ISO debug).
debug_iso: configure
	cmake --build $(BUILD_DIR) --target debug_iso

# 'make clean': Removes the build directory, effectively cleaning everything.
clean:
	rm -rf $(BUILD_DIR)

# 'make help': Displays the list of available commands.
help:
	@echo "TilekarOS Makefile Help"
	@echo "======================="
	@echo "Available commands:"
	@echo "  make             - Build the kernel and libc (default)"
	@echo "  make iso         - Build the bootable ISO image"
	@echo "  make run         - Run kernel directly in QEMU"
	@echo "  make run_iso     - Run ISO in QEMU"
	@echo "  make run_bochs   - Run ISO in Bochs"
	@echo "  make debug_run   - Debug kernel in QEMU with GDB"
	@echo "  make debug_iso   - Debug ISO in QEMU with GDB"
	@echo "  make clean       - Remove build artifacts"
	@echo "  make configure   - Re-run CMake configuration"
	@echo "  make help        - Show this help message"