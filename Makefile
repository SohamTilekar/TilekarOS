# ==============================================================================
# Wrapper Makefile for TilekarOS
# ==============================================================================

# The directory where build artifacts will be generated.
BUILD_DIR = build

# Emulation Workspace (Can be overridden by user: make run VM=MyCustomVM)
VM ?= VirtualMachine
DRIVE_DIR = $(VM)/drives

# Default Architecture
ARCH ?= i386

# Dynamic Drive Configuration
DRIVES ?= disk:10,disk2:5

# Arguments passed to CMake configuration step.
CMAKE_ARGS = -DOS_ARCH=$(ARCH) \
             -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/$(ARCH).cmake \
             -DDRIVE_CONFIG="$(DRIVES)" \
             -DVM_DIR="$(VM)"

.PHONY: all iso run run_iso run_bochs debug_run debug_iso clean configure help prepare_vm

# ------------------------------------------------------------------------------
# Targets
# ------------------------------------------------------------------------------

all: configure
	cmake --build $(BUILD_DIR)

configure:
	cmake -B $(BUILD_DIR) $(CMAKE_ARGS)

# Helper to setup the VM workspace and create/resize disk images
prepare_vm:
	@mkdir -p $(DRIVE_DIR)
	@echo "Preparing Workspace [$(VM)]: $(DRIVES)"
	@for entry in $$(echo "$(DRIVES)" | tr ',' ' '); do \
		name=$$(echo $$entry | cut -d: -f1); \
		size=$$(echo $$entry | cut -d: -f2); \
		if [ "$$name" = "$$size" ]; then size=""; fi; \
		img="$(DRIVE_DIR)/$$name.img"; \
		if [ -n "$$size" ]; then \
			if [ ! -f "$$img" ]; then \
				echo "  Creating $$img ($${size}MB)..."; \
				dd if=/dev/zero of="$$img" bs=1M count="$$size" status=none; \
			else \
				current_size=$$(stat -c%s "$$img" 2>/dev/null || echo 0); \
				target_size=$$(($$size * 1024 * 1024)); \
				if [ "$$current_size" -ne "$$target_size" ]; then \
					echo "  Resizing $$img to $${size}MB..."; \
					truncate -s "$${size}M" "$$img"; \
				fi; \
			fi; \
		else \
			if [ ! -f "$$img" ]; then \
				echo "Error: $$img not found and no size specified."; \
				exit 1; \
			fi; \
		fi; \
	done

run: configure prepare_vm
	cmake --build $(BUILD_DIR) --target run

run_iso: configure prepare_vm
	cmake --build $(BUILD_DIR) --target run_iso

run_bochs: configure
	cmake --build $(BUILD_DIR) --target run_bochs

debug_run: configure prepare_vm
	cmake --build $(BUILD_DIR) --target debug_run

debug_iso: configure prepare_vm
	cmake --build $(BUILD_DIR) --target debug_iso

clean:
	rm -rf $(BUILD_DIR)
	@echo "Cleaning workspace: $(VM)..."
	rm -rf $(VM)

help:
	@echo "TilekarOS Makefile Help"
	@echo "======================="
	@echo "Usage: make [target] VM=WorkspaceName DRIVES=name:size,..."
	@echo ""
	@echo "Available commands:"
	@echo "  make             - Build the kernel and libc"
	@echo "  make run         - Run in QEMU (Workspace: $(VM))"
	@echo "  make run_iso     - Run ISO in QEMU"
	@echo "  make clean       - Remove build artifacts and the '$(VM)' folder"
