# Architecture & Internals

This document provides a deep dive into the internal architecture of TilekarOS. It covers the boot sequence, memory organization, and the interrupt handling mechanism, supported by detailed diagrams.

## 1. The Boot Sequence

The transition from a powered-off machine to the executing kernel involves several stages. TilekarOS relies on a Multiboot-compliant bootloader (like GRUB) to handle the initial hardware setup.

### Detailed Boot Flowchart

```mermaid
flowchart TD
    %% --- Hardware / Bootloader ---
    subgraph SystemStart ["System Start"]
        direction TB
        BIOS["BIOS / UEFI"]:::hardware --> GRUB[GRUB Bootloader]:::hardware
        GRUB -- Multiboot Magic --> ProtMode["32-bit Protected Mode"]:::hardware
    end

    %% --- Low Level Entry ---
    subgraph BootASM ["boot.asm _start"]
        direction TB
        Stack["Setup Stack (esp = stack_top)"]:::asm
        Stack --> InitHub{Init Sequence}:::asm
    end

    SystemStart --> BootASM

    %% --- Modules ---
    
    %% TTY
    InitHub -- 1 --> TTY["tty.c: init_terminal"]:::c_module

    %% GDT
    subgraph GDT_Scope ["gdt.c: init_gdt"]
        direction LR
        G_Ld[Load GDTR]:::c_func --> G_TSS[Install TSS]:::c_func
        G_TSS --> G_TR[Load TR]:::c_func
    end
    InitHub -- 2 --> GDT_Scope

    %% IDT
    subgraph IDT_Scope ["idt.c: init_idt"]
        direction LR
        I_PIC["Remap PIC (0x20 / 0x28)"]:::c_func --> I_Fill[Fill Gates]:::c_func
        I_Fill --> I_Ld[Load IDTR]:::c_func
    end
    InitHub -- 3 --> IDT_Scope

    %% Kernel Main
    subgraph Kernel_Scope ["kernel.c: kernel_main"]
        direction TB
        AppLogic["Print Banner & Run Tests"]:::kernel --> Halt((Infinite Loop)):::hardware
    end
    InitHub -- 4 --> Kernel_Scope

    %% FORCE LEFT-TO-RIGHT ORDERING
    TTY ~~~ G_Ld ~~~ I_PIC ~~~ AppLogic
```

### Steps Explained

1.  **Bootloader Handoff**: GRUB ensures the CPU is in Protected Mode (A20 line enabled, Paging disabled). It places the **[Multiboot Information Structure](https://wiki.osdev.org/Multiboot)** in memory and puts the "Magic Number" in `EAX`.
2.  **Stack Setup**: Since C functions require a stack, `boot.asm` points the Stack Pointer (`ESP`) to the top of a reserved 16KiB block in the `.bss` section.
3.  **Global Descriptor Table (GDT)**: We replace the GDT provided by GRUB with our own to ensure we have full control over the memory segments and to set up the TSS (Task State Segment) for future ktask-mode switching. See [Global Descriptor Table](https://wiki.osdev.org/Global_Descriptor_Table).
4.  **Interrupt Descriptor Table (IDT)**: We configure the IDT to handle exceptions (like Divide-by-Zero) and hardware interrupts. The 8259 PIC is remapped to avoid IDT collisions. See [Interrupt Descriptor Table](https://wiki.osdev.org/Interrupt_Descriptor_Table).

---

## 2. Memory Organization

TilekarOS uses a **Flat Memory Model**. Paging is currently *disabled* (Linear Address = Physical Address). See [Memory Map (x86)](https://wiki.osdev.org/Memory_Map_(x86)).

### Memory Layout Diagram

```mermaid
block-beta
    columns 1
    
    block:high_mem
        space
        text["High Memory (Available for Allocation)"]
        space
    end

    block:kernel_space
        block:sections
            columns 3
            stack["Kernel Stack<br>(16 KB)"]
            bss[".bss Section<br>(Uninitialized)"]
            data[".data Section<br>(Initialized)"]
        end
        code[".text Section<br>(Kernel Code)"]
    end

    block:reserved
        vga["VGA Video Memory<br>(0xB8000 - 0xBFFFF)"]
        bios["BIOS Data Area & EBDA<br>(Reserved)"]
    end
```

### GDT Entry Structure

The GDT defines the characteristics of the various memory segments.

```mermaid
packet-beta
    0-15: "Limit (0-15)"
    16-31: "Base (0-15)"
    32-39: "Base (16-23)"
    40-47: "Access Byte"
    48-51: "Limit (16-19)"
    52-55: "Flags"
    56-63: "Base (24-31)"
```

*   **Base**: The starting linear address of the segment (0x0 for Flat Model).
*   **Limit**: The size of the segment.
*   **Access Byte**: Defines presence, privilege level (Ring 0 vs 3), and type (Code vs Data).
*   **Flags**: Granularity (4KiB blocks vs 1 Byte) and Operand size (32-bit vs 16-bit).

---

## 3. Interrupt Handling System

The interrupt system allows the CPU to pause current execution to handle urgent events (Exceptions, Hardware Signals, Syscalls).

### Interrupt Dispatch Pipeline

When an interrupt (e.g., IRQ 1 - Keyboard) occurs:

```mermaid
sequenceDiagram
    participant HW as Hardware (Keyboard)
    participant CPU
    participant IDT as IDT (Table)
    participant ASM as ASM Wrapper (isr_common)
    participant C as C Handler (irq_handler)

    HW->>CPU: Signals Interrupt (IRQ 1)
    Note over CPU: 1. Push EFLAGS, CS, EIP<br>2. Read IDT Entry 33
    CPU->>ASM: Jump to ISR Stub (irq1)
    activate ASM
    ASM->>ASM: Push Interrupt Number (33)
    ASM->>ASM: PushAll (EAX, ECX, EDX...)
    ASM->>ASM: Load Kernel Data Segment
    ASM->>C: Call irq_handler(registers)
    activate C
    C->>C: Look up registered handler
    C-->>C: Handle Event (Read Scancode)
    C->>HW: Send EOI (End of Interrupt)
    C->>ASM: Return
    deactivate C
    ASM->>ASM: Restore Registers (PopAll)
    ASM->>CPU: IRET (Interrupt Return)
    deactivate ASM
    CPU->>HW: Resume Execution
```

### IDT Configuration

The IDT is an array of 256 8-byte entries.

| Vector Range | Usage | Description |
| :--- | :--- | :--- |
| **0 - 31** | **CPU Exceptions** | Faults, Traps, and Aborts (e.g., Page Fault, GPF). |
| **32 - 47** | **Hardware IRQs** | Remapped PIC interrupts. IRQ0=32 (Timer), IRQ1=33 (Keyboard). |
| **48 - 127** | *Reserved* | Available for custom use. |
| **128 (0x80)** | **Syscall** | Typical Linux-style system call entry point. |
| **177** | **Syscall** | Alternate system call entry point. |

### The PIC Remapping Issue
By default, the [8259 PIC](https://wiki.osdev.org/8259_PIC) maps IRQs 0-7 to Interrupt Vectors 0-7. This conflicts with CPU Exceptions (e.g., INT 0 is Divide-by-Zero).
**Solution**: We reprogram the PIC to offset IRQs to start at vector **32 (0x20)**.

*   Master PIC (IRQs 0-7) $\rightarrow$ Vectors 32-39
*   Slave PIC (IRQs 8-15) $\rightarrow$ Vectors 40-47

---

## 4. Kernel Execution & Testing

The `kernel_main` function serves as the entry point for the OS after initialization. Currently, it performs the following actions:

1.  **Banner Display**: Prints the OS welcome message using `terminal_writestring`.
2.  **Exception Test**: Intentionally triggers a **Divide-by-Zero** exception (`1/0`).
    *   **Purpose**: To verify that the IDT is correctly loaded and the ISR for Vector 0 is firing.
    *   **Expected Behavior**: The OS should catch the exception, print a debug message (if implemented), and halt or panic, rather than triple-faulting or resetting.
