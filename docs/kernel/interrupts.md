# Interrupts & Exceptions

The Interrupt system is the backbone of TilekarOS, handling hardware events, CPU faults, and system calls.

## 1. The IDT (Interrupt Descriptor Table)

The IDT is an array of 256 gates. Each gate points to an assembly "stub" that handles a specific interrupt.

### IDT Structures
- **`IDTEntry`**: Each entry is 8 bytes. It contains the 32-bit address of the ISR (split into high/low words), the kernel code segment selector (`0x08`), and attributes.
- **Attributes**: Bits 0-3 (Gate Type), Bit 4 (S), Bits 5-6 (DPL), Bit 7 (P).
- **`IDTR`**: A 6-byte structure used by the `lidt` instruction, containing the limit (size - 1) and the linear address of the IDT array.

### Vector Layout
- **0 - 31**: CPU Exceptions (Faults/Traps).
  - 0: Divide-by-Zero
  - 13: General Protection Fault
  - 14: Page Fault (presents faulting address in `CR2`).
- **32 - 47**: Hardware Interrupts (IRQs).
  - 32: Timer (PIT)
  - 33: Keyboard
- **128 (0x80)**: System Call Entry.

---

## 2. Programmable Interrupt Controller (PIC)

TilekarOS remaps the 8259 PIC so that IRQs start at index 32, avoiding conflicts with CPU exceptions.

### PIC Mapping
- **Master PIC**: Remapped to **0x20** (32). Handles IRQs 0-7.
- **Slave PIC**: Remapped to **0x28** (40). Handles IRQs 8-15.

### End of Interrupt (EOI)
After handling an IRQ, the kernel MUST send an EOI signal (0x20) to the PIC.

- If the IRQ was on the **Slave PIC** (>= 40), an EOI must be sent to *both* the Slave and the Master.
- If the IRQ was only on the **Master PIC**, an EOI is only sent to the Master.

---

## 3. Interrupt Lifecycle

  1. **Hardware/CPU Triggers**: CPU pushes `EFLAGS`, `CS`, and `EIP`. If a privilege switch occurred, `SS` and `ESP` are also pushed.
  2. **Assembly Stub (`idt.asm`)**:
    - Pushes an error code (or 0 if none).
    - Pushes the interrupt number.
    - Saves all registers (`pushad`).
    - Saves segment registers (`gs`, `fs`, `es`, `ds`).
    - Sets `DS` and `ES` to the kernel data segment.
  3. **C Handler**: `irq_handler` or `isr_handler` is called.
    - All registers are accessible via the `InteruptReg` pointer.
  4. **Restore**: Registers are restored (`popad`), and `iret` returns to the previous execution flow.

---

## 4. Handler Registration

Kernel modules can register handlers for specific IRQs using `irq_install_handler(irq_no, handler)`.

---

## 5. Register Context (`InteruptReg`)

All handlers receive a pointer to the saved registers on the stack:
```c
typedef struct {
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t intr_num, err_cod;
    uint32_t eip, csm, eflags, useresp, ss;
} InteruptReg;
```
