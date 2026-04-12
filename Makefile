# ==============================================================================
# TilekarOS Master Makefile
# ==============================================================================

# ------------------------------------------------------------------------------
# Configuration & Defaults
# ------------------------------------------------------------------------------

BUILD_DIR = build
VM ?= VirtualMachine
DRIVE_DIR = $(VM)/drives
ARCH ?= i386
DRIVES ?= boot:24:ide
SYSROOT = sysroot

CMAKE_ARGS = -DOS_ARCH=$(ARCH) \
             -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/$(ARCH).cmake

# Persist VM and DRIVES if provided
ifneq ($(filter command line environment,$(origin VM)),)
    CMAKE_ARGS += -DVM_DIR="$(VM)"
endif
ifneq ($(filter command line environment,$(origin DRIVES)),)
    CMAKE_ARGS += -DDRIVE_CONFIG="$(DRIVES)"
endif

# ------------------------------------------------------------------------------
# Primary Targets
# ------------------------------------------------------------------------------

.PHONY: all kernel iso sysroot full_build clean help prepare_vm run run_iso run_disk debug_run debug_iso export_drives comp_exe configure disk_img

all: kernel sysroot

kernel: configure
	@echo "--- Building Kernel ---"
	cmake --build $(BUILD_DIR) --target myos.kernel

iso: configure
	@echo "--- Generating ISO ---"
	cmake --build $(BUILD_DIR) --target iso

sysroot: kernel
	@echo "--- Populating Sysroot ---"
	cmake --build $(BUILD_DIR) --target sysroot_extras

full_build: all iso

# Internal target to create bootable folder structure
_prepare_isodir: all
	@rm -rf $(BUILD_DIR)/disk_isodir
	@mkdir -p $(BUILD_DIR)/disk_isodir/boot/grub
	@cp $(BUILD_DIR)/kernel/myos.kernel $(BUILD_DIR)/disk_isodir/boot/
	@cp grub.cfg $(BUILD_DIR)/disk_isodir/boot/grub/
	@if [ -d "$(VM)/exported_drives/boot" ]; then \
		cp -r $(VM)/exported_drives/boot/* $(BUILD_DIR)/disk_isodir/ 2>/dev/null || true; \
	fi
	@for cfile in $$(find "$(VM)" -maxdepth 1 -name "*.c" 2>/dev/null); do \
		if [ -f "$$cfile" ]; then \
			filename=$$(basename "$$cfile" .c); \
			cp "$$cfile" $(BUILD_DIR)/disk_isodir/"$$filename"; \
		fi; \
	done

# Create a standalone bootable disk image
disk_img: _prepare_isodir
	@echo "--- Generating Standalone Bootable Disk Image ---"
	grub-mkrescue -o $(BUILD_DIR)/tilekaros.img $(BUILD_DIR)/disk_isodir

# ------------------------------------------------------------------------------
# Build Infrastructure
# ------------------------------------------------------------------------------

configure:
	@mkdir -p $(BUILD_DIR)
	cmake -B $(BUILD_DIR) $(CMAKE_ARGS)

# ------------------------------------------------------------------------------
# Userspace Development
# ------------------------------------------------------------------------------

comp_exe: all
	@if [ -z "$(FILE)" ]; then echo "Usage: make comp_exe FILE=src.c"; exit 1; fi
	@if [ -z "$(OUT)" ]; then OUT=$$(basename $(FILE) .c); fi; \
	mkdir -p $$(dirname $(OUT)); \
	clang --target=$(ARCH)-elf --sysroot=$(SYSROOT) -nostdlib -ffreestanding -fno-pic -fno-pie -static -O2 -Wall \
		-Wl,-z,noexecstack -Wl,--build-id=none -T $(SYSROOT)/usr/lib/user.ld \
		$(SYSROOT)/usr/lib/crt0.o $(FILE) $(SYSROOT)/usr/lib/libc.a -o $(OUT)

# ------------------------------------------------------------------------------
# VM Management & Emulation
# ------------------------------------------------------------------------------

prepare_vm:
	@mkdir -p $(DRIVE_DIR)
	@mkdir -p $(VM)/exported_drives
	@echo "Preparing Workspace [$(VM)]: $(DRIVES)"
	@boot_drive=$$(echo "$(DRIVES)" | tr ',' ' ' | head -n1 | cut -d: -f1); \
	for entry in $$(echo "$(DRIVES)" | tr ',' ' '); do \
		name=$$(echo $$entry | cut -d: -f1); \
		size=$$(echo $$entry | cut -d: -f2); \
		[ "$$name" = "$$size" ] && size=""; \
		[ -z "$$size" ] && size=24; \
		img="$(DRIVE_DIR)/$$name.img"; \
		export_dir="$(VM)/exported_drives/$$name"; \
		if [ -d "$$export_dir" ] && [ ! -f "$$img.bootable" ]; then \
			echo "  Recreating $$img from $$export_dir..."; \
			dd if=/dev/zero of="$$img" bs=1M count="$$size" status=none; \
			mkfs.fat "$$img" > /dev/null; \
			if [ -n "$$(ls -A "$$export_dir" 2>/dev/null)" ]; then \
				mcopy -i "$$img" -s "$$export_dir"/* ::/ 2>/dev/null || true; \
			fi; \
		elif [ ! -f "$$img" ]; then \
			echo "  Creating empty $$img ($${size}MB)..."; \
			dd if=/dev/zero of="$$img" bs=1M count="$$size" status=none; \
			mkfs.fat "$$img" > /dev/null; \
		fi; \
		if [ "$$name" = "$$boot_drive" ] && [ ! -f "$$img.bootable" ]; then \
			echo "  Updating kernel and sources on $$img..."; \
			mmd -i "$$img" ::/boot 2>/dev/null || true; \
			mcopy -i "$$img" -D o "$(BUILD_DIR)/kernel/myos.kernel" ::/boot/myos.kernel 2>/dev/null || true; \
			for cfile in $$(find "$(VM)" -maxdepth 1 -name "*.c" 2>/dev/null); do \
				if [ -f "$$cfile" ]; then \
					filename=$$(basename "$$cfile" .c); \
					mcopy -i "$$img" -D o "$$cfile" ::/"$$filename" 2>/dev/null || true; \
				fi; \
			done; \
		fi; \
	done

export_drives:
	@mkdir -p $(VM)/exported_drives
	@for entry in $$(echo "$(DRIVES)" | tr ',' ' '); do \
		name=$$(echo $$entry | cut -d: -f1); \
		img="$(DRIVE_DIR)/$$name.img"; \
		export_dir="$(VM)/exported_drives/$$name"; \
		if [ -f "$$img" ]; then \
			mkdir -p "$$export_dir"; \
			if minfo -i "$$img" :: 2>/dev/null; then \
				mcopy -i "$$img" -snD o "::/*" "$$export_dir"/ 2>/dev/null || true; \
			else \
				mcopy -i "$$img"@@512 -snD o "::/*" "$$export_dir"/ 2>/dev/null || true; \
			fi; \
		fi; \
	done

run: all prepare_vm
	@rm -f $(DRIVE_DIR)/*.bootable
	cmake --build $(BUILD_DIR) --target run
	$(MAKE) export_drives

run_iso: all iso prepare_vm
	@rm -f $(DRIVE_DIR)/*.bootable
	cmake --build $(BUILD_DIR) --target run_iso
	$(MAKE) export_drives

run_disk: _prepare_isodir
	@boot_drive=$$(echo "$(DRIVES)" | tr ',' ' ' | head -n1 | cut -d: -f1); \
	img="$(DRIVE_DIR)/$$boot_drive.img"; \
	mkdir -p $(DRIVE_DIR); \
	echo "--- Building Bootable Drive ($$img) ---"; \
	grub-mkrescue -o "$$img" $(BUILD_DIR)/disk_isodir; \
	touch "$$img.bootable"; \
	$(MAKE) prepare_vm; \
	cmake --build $(BUILD_DIR) --target run_disk; \
	$(MAKE) export_drives

debug_run: configure prepare_vm
	cmake --build $(BUILD_DIR) --target debug_run
	$(MAKE) export_drives

debug_iso: iso prepare_vm
	cmake --build $(BUILD_DIR) --target debug_iso
	$(MAKE) export_drives

# ------------------------------------------------------------------------------
# Help & Cleanup
# ------------------------------------------------------------------------------

clean:
	rm -rf $(BUILD_DIR) $(SYSROOT)
	rm -rf $(VM)

help:
	@echo "================================================================================"
	@echo "🌌 TilekarOS Build System Help"
	@echo "================================================================================"
	@echo ""
	@echo "🛠️  BUILD TARGETS:"
	@echo "  make             - Default: Build kernel and populate development sysroot"
	@echo "  make kernel      - Build only the kernel binary"
	@echo "  make sysroot     - Create/update the userspace development environment"
	@echo "  make iso         - Generate a bootable ISO image (build/myos.iso)"
	@echo "  make disk_img    - Generate a standalone bootable disk image (build/tilekaros.img)"
	@echo "  make full_build  - Build everything: Kernel, ISO, and Integrated Disk"
	@echo "  make clean       - Remove all build artifacts, sysroot, and current VM folder"
	@echo ""
	@echo "🚀 EMULATION TARGETS:"
	@echo "  make run         - Kernel Mode: Loads kernel directly (fastest for dev)"
	@echo "  make run_disk    - Disk Mode: Boots from primary drive using GRUB (realistic)"
	@echo "  make run_iso     - ISO Mode: Boots from virtual CD-ROM"
	@echo "  make debug_run   - Run in Kernel Mode with GDB debugger attached"
	@echo ""
	@echo "💻 USERSPACE DEVELOPMENT:"
	@echo "  make comp_exe FILE=src.c [OUT=bin]"
	@echo "                   - Compile a C application using the TilekarOS sysroot."
	@echo "                     Automatically links against LibC and sets entry point."
	@echo ""
	@echo "⚙️  VARIABLES (Can be passed to any 'make run' command):"
	@echo "  VM=Name          - Specify the workspace folder (Default: VirtualMachine)"
	@echo "  DRIVES=cfg       - Configure disk images (Default: boot:24:ide)"
	@echo "                     Format: name:size_in_mb:interface (ide/ahci)"
	@echo "                     Example: make run_disk VM=dump DRIVES=boot:32:ahci,data:10:ide"
	@echo ""
	@echo "📂 WORKSPACE FEATURES:"
	@echo "  - Any .c files in your VM folder are automatically placed in the boot drive root."
	@echo "  - Files written by the OS are exported to $(VM)/exported_drives/ on exit."
	@echo "================================================================================"
