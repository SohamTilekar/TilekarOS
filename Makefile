# Root Makefile
# Delegates all commands to the kernel/Makefile

.PHONY: all iso run run_iso debug rund clean

all:
	$(MAKE) -C kernel all

iso:
	$(MAKE) -C kernel iso

run:
	$(MAKE) -C kernel run

run_iso:
	$(MAKE) -C kernel run_iso

debug:
	$(MAKE) -C kernel debug

rund:
	$(MAKE) -C kernel rund

run_isod:
	$(MAKE) -C kernel run_isod

clean:
	$(MAKE) -C kernel clean
