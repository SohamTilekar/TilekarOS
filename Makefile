ARCH ?= i386
VM ?= VirtualMachine
DRIVES ?= boot:24:ide
PYTHON ?= python3
BUILD_PY := helpers/build.py

.PHONY: all configure kernel sysroot userland iso run run_test run_iso run_disk export_drives comp comp_exe clean help

all: kernel sysroot userland

configure:
	$(PYTHON) $(BUILD_PY) configure --arch $(ARCH)

kernel:
	$(PYTHON) $(BUILD_PY) kernel --arch $(ARCH)

sysroot:
	$(PYTHON) $(BUILD_PY) sysroot --arch $(ARCH)

userland:
	$(PYTHON) $(BUILD_PY) userland --arch $(ARCH)

iso:
	$(PYTHON) $(BUILD_PY) iso --arch $(ARCH)

run:
	$(PYTHON) $(BUILD_PY) run --arch $(ARCH) --vm "$(VM)" --drives "$(DRIVES)"

run_test:
	$(PYTHON) $(BUILD_PY) run --arch $(ARCH) --vm "$(VM)" --drives "$(DRIVES)" --enable-test

run_iso:
	$(PYTHON) $(BUILD_PY) run_iso --arch $(ARCH) --vm "$(VM)" --drives "$(DRIVES)"

run_disk:
	$(PYTHON) $(BUILD_PY) run_disk --arch $(ARCH) --vm "$(VM)" --drives "$(DRIVES)"

export_drives:
	$(PYTHON) $(BUILD_PY) export_drives --vm "$(VM)" --drives "$(DRIVES)"

comp:
	@if [ -z "$(FILE)" ]; then echo "Usage: make comp FILE=src.c [OUT=bin]"; exit 1; fi
	$(PYTHON) $(BUILD_PY) comp --arch $(ARCH) --file "$(FILE)" --out "$(OUT)"

comp_exe: comp

clean:
	$(PYTHON) $(BUILD_PY) clean --vm "$(VM)"

help:
	@echo "TilekarOS Build (make -> python -> cmake)"
	@echo "  make|make all               Build kernel + sysroot + userland"
	@echo "  make kernel|sysroot|userland|iso Build selected artifact"
	@echo "  make run|run_test|run_iso|run_disk   Run QEMU with VM drives"
	@echo "    run                       Run without tests (interactive prompt)"
	@echo "    run_test                  Run with tests enabled (auto-run)"
	@echo "  make comp FILE=app.c [OUT=app]  Compile userspace app with sysroot+libc"
	@echo "  make export_drives          Export VM drive contents"
	@echo "  make clean                  Remove build/sysroot and selected VM directory"
	@echo ""
	@echo "Variables:"
	@echo "  ARCH=$(ARCH) VM=$(VM) DRIVES=$(DRIVES)"
	@echo "  DRIVES format: name:size:attach_type{ide,ahci},..."
