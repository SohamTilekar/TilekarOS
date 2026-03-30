# Hardware Drivers Reference

This document provides a technical overview of the hardware drivers implemented in TilekarOS.

## 1. VGA & TTY Driver
**Source Files**: [tty.c](https://github.com/SohamTilekar/TilekarOS/blob/main/kernel/arch/i386/drivers/tty.c){: target="_blank" }, [vga.h](https://github.com/SohamTilekar/TilekarOS/blob/main/kernel/arch/i386/drivers/vga.h){: target="_blank" }

The VGA driver manages the text-mode buffer at `0xB8000`.

### Features:
- **Scrolling**: Moves the entire screen up when the bottom is reached.
- **Color Support**: Supports 16 foreground and 16 background colors.
- **Hardware Cursor**: Updates the blinking underscore using VGA I/O ports.
- **Serial Logging**: Mirrors all output to the **COM1 Serial Port** (`0x3F8`) for easy debugging.

---

## 2. PS/2 Keyboard Driver
**Source Files**: [keyboard.c](https://github.com/SohamTilekar/TilekarOS/blob/main/kernel/arch/i386/drivers/keyboard.c){: target="_blank" }, [keyboard.h](https://github.com/SohamTilekar/TilekarOS/blob/main/kernel/arch/i386/drivers/keyboard.h){: target="_blank" }

The keyboard driver handles input from a standard PS/2 keyboard using IRQ 1.

### Features:
- **Scancode Mapping**: Translates raw PS/2 Set 1 scancodes into ASCII characters.
- **Modifier Tracking**: Tracks the state of Shift, Ctrl, Alt, and Caps Lock.
- **Buffer API**: Provides `keyboard_getchar()` which yields until a key is pressed.

---

## 3. Programmable Interval Timer (PIT)
**Source Files**: [timer.c](https://github.com/SohamTilekar/TilekarOS/blob/main/kernel/arch/i386/drivers/timer.c){: target="_blank" }, [timer.h](https://github.com/SohamTilekar/TilekarOS/blob/main/kernel/arch/i386/drivers/timer.h){: target="_blank" }

The PIT generates periodic interrupts at a fixed frequency (default: 1000Hz).

### The Trigger System:
TilekarOS features a "Trigger" system that allows functions to be scheduled to run every X ticks.
- **Preemption**: The scheduler uses a trigger to preempt tasks every 100ms.
- **Timekeeping**: Global `ticks` counter since boot.

---

## 4. PCI Bus Driver
**Source Files**: [pci.c](https://github.com/SohamTilekar/TilekarOS/blob/main/kernel/arch/i386/drivers/pci.c){: target="_blank" }, [pci.h](https://github.com/SohamTilekar/TilekarOS/blob/main/kernel/arch/i386/drivers/pci.h){: target="_blank" }

The PCI driver enumerates devices on the PCI bus using I/O ports `0xCF8` (Address) and `0xCFC` (Data).

### Features:
- **Device Enumeration**: Scans all 32 devices on each of the 256 buses.
- **Vendor/Device ID**: Identifies hardware manufacturers and models.
- **Class/Subclass**: Determines device types (e.g., Storage Controller, Display Adapter).

---

## 5. ATA (IDE) Disk Driver
**Source Files**: [ata.c](https://github.com/SohamTilekar/TilekarOS/blob/main/kernel/arch/i386/drivers/ata.c){: target="_blank" }, [ata.h](https://github.com/SohamTilekar/TilekarOS/blob/main/kernel/arch/i386/drivers/ata.h){: target="_blank" }

The ATA driver provides access to IDE hard drives using both PIO (Programmed I/O) and **DMA (Direct Memory Access)** modes.

### Features:
- **LBA28 Support**: Supports disks up to 128GB.
- **Bus Master DMA**: Uses the PCI Bus Master IDE (BMIDE) interface for high-speed transfers.
- **PRDT Management**: Automatically manages Physical Region Descriptor Tables for DMA transfers.
- **Interrupt Driven**: Uses IRQ 14 (Primary) and IRQ 15 (Secondary) for asynchronous completion.
- **Fallback Mechanism**: Automatically falls back to PIO mode if DMA is unavailable.

### DMA Implementation:
1.  **PCI Configuration**: The kernel enables "Bus Mastering" in the PCI Command register for the IDE controller.
2.  **PRDT Setup**: A Physical Region Descriptor Table is prepared, pointing to a physically contiguous DMA buffer.
3.  **BMIDE Control**: The BMIDE Command register is used to start/stop the transfer, while the Status register reports completion or errors.

---

## References
- [OSDev: VGA Text Mode](https://wiki.osdev.org/VGA_Text_Mode)
- [OSDev: PS/2 Keyboard](https://wiki.osdev.org/PS/2_Keyboard)
- [OSDev: PIT](https://wiki.osdev.org/Programmable_Interval_Timer)
- [OSDev: PCI](https://wiki.osdev.org/PCI)
- [OSDev: ATA PIO Mode](https://wiki.osdev.org/ATA_PIO_Mode)
