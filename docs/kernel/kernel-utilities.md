# Kernel Utility Functions

TilekarOS provides a set of low-level utilities to interact with the CPU and hardware.

## 1. I/O Port Access

These functions are used to communicate with hardware controllers (like the PIC, PIT, and Keyboard).

- `out_port_b(port, value)`: Sends a byte to an I/O port.
- `in_port_b(port)`: Reads a byte from an I/O port.

## 2. Interrupt Control

Atomic operations are implemented using these functions to prevent race conditions during sensitive kernel sections.

```c
/**
 * interrupt_save - Saves the current EFLAGS and disables interrupts (cli).
 * Return: The saved EFLAGS value.
 */
uint32_t flags = interrupt_save();

// --- CRITICAL SECTION ---

/**
 * interrupt_restore - Restores the EFLAGS (and thus interrupt state).
 */
interrupt_restore(flags);
```

### Implementation Details:
- **`interrupt_save`**: Uses `pushfl` to push the EFLAGS register onto the stack, `popl` to store it in a C variable, and then executes `cli`.
- **`interrupt_restore`**: Uses `pushl` to push the saved EFLAGS back onto the stack and `popfl` to restore the CPU state.

---

## 3. Register Context (`InteruptReg`)

When an interrupt or exception occurs, the CPU and assembly stubs save a complete snapshot of the registers on the stack.

### Important Fields:
- **`gs`, `fs`, `es`, `ds`**: Pushed by segment registers.
- **`edi`, `esi`, `ebp`, `esp`, `ebx`, `edx`, `ecx`, `eax`**: Pushed by `pushad`.
- **`intr_num`**: The interrupt number being handled (0-255).
- **`err_cod`**: Optional error code from the CPU (e.g., from Page Fault).
- **`eip`, `csm`, `eflags`**: Current instruction pointer and CPU state.
- **`useresp`, `ss`**: Only present if the interrupt occurred in **User Mode**.

---

## 4. Helper Macros

- **`CEIL_DIV(a, b)`**: Calculates `(a + b - 1) / b`. This is commonly used in memory management for calculating the number of pages required for a given size.
