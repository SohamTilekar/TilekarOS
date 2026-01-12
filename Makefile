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

.PHONY: all iso run run_iso clean configure

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

# 'make clean': Removes the build directory, effectively cleaning everything.
clean:
	rm -rf $(BUILD_DIR)