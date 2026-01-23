#include "local_config.h"
#include <kernel/tty.h>
#include <stdint.h>
#include <string.h>
#include "idt.h"
#include "utils.h"

/*
 * IDT Entry Structure
 * Defines the format of a single interrupt gate as required by the x86 CPU.
 * Used to tell the CPU where to jump when an interrupt occurs.
 */
typedef struct
{
    uint16_t isr_low;               // The lower 16 bits of the ISR's address
    uint16_t kernel_cs;             // The GDT Code Segment selector (tells CPU which segment the ISR lives in)
    uint8_t reserved;               // Must be set to zero (reserved by Intel)
    uint8_t attributes;             // Type and attributes (Present, DPL, Gate Type)
    uint16_t isr_high;              // The higher 16 bits of the ISR's address
} __attribute__((packed)) IDTEntry; // 'packed' prevents compiler padding, ensuring exact hardware alignment

/*
 * IDT Flags
 * Constants to set the 'attributes' byte in the IDTEntry.
 */
enum IDT_FLAGS
{
    IDT_FLAG_PRESENT = 0x80,   // Bit 7: 1 = Interrupt is present
    IDT_FLAG_RING0 = 0x00,     // Bits 5-6: Privilege Level 0 (Kernel)
    IDT_FLAG_RING1 = 0x20,     // Bits 5-6: Privilege Level 1
    IDT_FLAG_RING2 = 0x40,     // Bits 5-6: Privilege Level 2
    IDT_FLAG_RING3 = 0x60,     // Bits 5-6: Privilege Level 3 (User)
    IDT_FLAG_TASK_GATE = 0x05, // Gate Type: Task Gate
    IDT_FLAG_INT_GATE = 0x0E,  // Gate Type: 32-bit Interrupt Gate
    IDT_FLAG_TRAP_GATE = 0x0F, // Gate Type: 32-bit Trap Gate
};

/*
 * IDT Register (IDTR)
 * The structure pointer required by the 'lidt' assembly instruction.
 */
typedef struct
{
    uint16_t limit; // Size of the IDT in bytes - 1
    uint32_t base;  // Linear address where the IDT array starts
} __attribute__((packed)) IDTR;

// The actual Interrupt Descriptor Table array (256 entries).
// Aligned to 0x10 for potential performance optimization on some CPUs.
__attribute__((aligned(0x10))) static IDTEntry idt[256];

// The IDTR structure telling the CPU the size of our table.
static IDTR idtr = {
    .limit = sizeof(idt) - 1,
};

// Assembly helper to load the IDT pointer (lidt instruction)
extern void idt_flush(uint32_t);


/*
 * External Assembly Interrupt Service Routines (ISRs)
 * These are defined in an assembly file (e.g., interrupt.asm).
 * They prepare the stack and call the C handlers.
 */

// --- CPU Exceptions (0-31) ---
extern void isr0();  // Divide by Zero
extern void isr1();  // Debug
extern void isr2();  // NMI
extern void isr3();  // Breakpoint
extern void isr4();  // Overflow
extern void isr5();  // Bound Range Exceeded
extern void isr6();  // Invalid Opcode
extern void isr7();  // Device Not Available
extern void isr8();  // Double Fault
extern void isr9();  // Coprocessor Segment Overrun
extern void isr10(); // Invalid TSS
extern void isr11(); // Segment Not Present
extern void isr12(); // Stack-Segment Fault
extern void isr13(); // General Protection Fault
extern void isr14(); // Page Fault
extern void isr15(); // Reserved
extern void isr16(); // x87 Floating Point Exception
extern void isr17(); // Alignment Check
extern void isr18(); // Machine Check
extern void isr19(); // SIMD Floating Point Exception
extern void isr20(); // Virtualization Exception
extern void isr21(); // Reserved
extern void isr22(); // Reserved
extern void isr23(); // Reserved
extern void isr24(); // Reserved
extern void isr25(); // Reserved
extern void isr26(); // Reserved
extern void isr27(); // Reserved
extern void isr28(); // Reserved
extern void isr29(); // Reserved
extern void isr30(); // Security Exception
extern void isr31(); // Reserved

// --- Software Interrupts / System Calls ---
extern void isr128(); // Often used for Syscalls (0x80)
extern void isr177(); // Alternative Syscall entry

// --- Hardware Interrupts (IRQs) ---
// Mapped from PIC hardware lines to IDT entries 32-47
extern void irq0();  // Timer
extern void irq1();  // Keyboard
extern void irq2();  // Cascade (internal for PIC2)
extern void irq3();  // COM2
extern void irq4();  // COM1
extern void irq5();  // LPT2
extern void irq6();  // Floppy Disk
extern void irq7();  // LPT1
extern void irq8();  // CMOS real-time clock
extern void irq9();  // Legacy SCSI / NIC
extern void irq10(); // SCSI / NIC
extern void irq11(); // SCSI / NIC
extern void irq12(); // PS/2 Mouse
extern void irq13(); // FPU
extern void irq14(); // Primary ATA Hard Disk
extern void irq15(); // Secondary ATA Hard Disk

/*
 * Helper: Set IDT Gate
 * Fills an IDT entry with the address of the handler and settings.
 */
void set_idt_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags)
{
    idt[num].isr_low = base & 0xFFFF;          // Low 16 bits of address
    idt[num].isr_high = (base >> 16) & 0xFFFF; // High 16 bits of address
    idt[num].kernel_cs = sel;                  // Code Segment Selector
    idt[num].reserved = 0;                     // Always 0
    idt[num].attributes = flags;               // Flags (Present, Ring, Type)
};

/*
 * Initialize IDT
 * Sets up the table, remaps the PIC, and loads the IDT into the CPU.
 */
void init_idt()
{
    // 1. Set the base address in the IDT register pointer
    idtr.base = (uint32_t)&idt[0];

    // 2. Clear the IDT memory area
    memset    (&idt, 0, sizeof(idt));

    /*
     * 3. Remap the Programmable Interrupt Controller (PIC).
     * By default, IRQs 0-7 map to interrupts 0-7, which conflicts with
     * CPU exceptions (like "Division by Zero"). We move them to 32-47.
     */

    // ICW1: Start initialization sequence (in cascade mode)
    out_port_b(0x20, 0x11); // Master PIC
    out_port_b(0xA0, 0x11); // Slave PIC

    // ICW2: Vector Offsets (Remapping happens here)
    out_port_b(0x21, 0x20); // Master PIC vector offset start = 32 (0x20)
    out_port_b(0xA1, 0x28); // Slave PIC vector offset start = 40 (0x28)

    // ICW3: Cascade identity
    out_port_b(0x21, 0x04); // Tell Master that Slave is at IRQ2 (0000 0100)
    out_port_b(0xA1, 0x02); // Tell Slave its cascade identity (0000 0010)

    // ICW4: 8086 mode
    out_port_b(0x21, 0x01);
    out_port_b(0xA1, 0x01);

    // OCW1: Unmask all interrupts (allow them to happen)
    out_port_b(0x21, 0x0);
    out_port_b(0xA1, 0x0);

    /*
     * 4. Populate the IDT
     * Connect the assembly stub functions to the IDT slots.
     * Note: GDT_KERNEL_CS_OFFSET is usually 0x08 (Kernel Code Segment).
     */

    // --- Exception Gates (0-31) ---
    set_idt_gate( 0, (uint32_t)isr0 , GDT_KERNEL_CS_OFFSET, IDT_FLAG_PRESENT | IDT_FLAG_RING3 | IDT_FLAG_INT_GATE);
    set_idt_gate( 1, (uint32_t)isr1 , GDT_KERNEL_CS_OFFSET, IDT_FLAG_PRESENT | IDT_FLAG_RING3 | IDT_FLAG_INT_GATE);
    set_idt_gate( 2, (uint32_t)isr2 , GDT_KERNEL_CS_OFFSET, IDT_FLAG_PRESENT | IDT_FLAG_RING3 | IDT_FLAG_INT_GATE);
    set_idt_gate( 3, (uint32_t)isr3 , GDT_KERNEL_CS_OFFSET, IDT_FLAG_PRESENT | IDT_FLAG_RING3 | IDT_FLAG_INT_GATE);
    set_idt_gate( 4, (uint32_t)isr4 , GDT_KERNEL_CS_OFFSET, IDT_FLAG_PRESENT | IDT_FLAG_RING3 | IDT_FLAG_INT_GATE);
    set_idt_gate( 5, (uint32_t)isr5 , GDT_KERNEL_CS_OFFSET, IDT_FLAG_PRESENT | IDT_FLAG_RING3 | IDT_FLAG_INT_GATE);
    set_idt_gate( 6, (uint32_t)isr6 , GDT_KERNEL_CS_OFFSET, IDT_FLAG_PRESENT | IDT_FLAG_RING3 | IDT_FLAG_INT_GATE);
    set_idt_gate( 7, (uint32_t)isr7 , GDT_KERNEL_CS_OFFSET, IDT_FLAG_PRESENT | IDT_FLAG_RING3 | IDT_FLAG_INT_GATE);
    set_idt_gate( 8, (uint32_t)isr8 , GDT_KERNEL_CS_OFFSET, IDT_FLAG_PRESENT | IDT_FLAG_RING3 | IDT_FLAG_INT_GATE);
    set_idt_gate( 9, (uint32_t)isr9 , GDT_KERNEL_CS_OFFSET, IDT_FLAG_PRESENT | IDT_FLAG_RING3 | IDT_FLAG_INT_GATE);
    set_idt_gate(10, (uint32_t)isr10, GDT_KERNEL_CS_OFFSET, IDT_FLAG_PRESENT | IDT_FLAG_RING3 | IDT_FLAG_INT_GATE);
    set_idt_gate(11, (uint32_t)isr11, GDT_KERNEL_CS_OFFSET, IDT_FLAG_PRESENT | IDT_FLAG_RING3 | IDT_FLAG_INT_GATE);
    set_idt_gate(12, (uint32_t)isr12, GDT_KERNEL_CS_OFFSET, IDT_FLAG_PRESENT | IDT_FLAG_RING3 | IDT_FLAG_INT_GATE);
    set_idt_gate(13, (uint32_t)isr13, GDT_KERNEL_CS_OFFSET, IDT_FLAG_PRESENT | IDT_FLAG_RING3 | IDT_FLAG_INT_GATE);
    set_idt_gate(14, (uint32_t)isr14, GDT_KERNEL_CS_OFFSET, IDT_FLAG_PRESENT | IDT_FLAG_RING3 | IDT_FLAG_INT_GATE);
    set_idt_gate(15, (uint32_t)isr15, GDT_KERNEL_CS_OFFSET, IDT_FLAG_PRESENT | IDT_FLAG_RING3 | IDT_FLAG_INT_GATE);
    set_idt_gate(16, (uint32_t)isr16, GDT_KERNEL_CS_OFFSET, IDT_FLAG_PRESENT | IDT_FLAG_RING3 | IDT_FLAG_INT_GATE);
    set_idt_gate(17, (uint32_t)isr17, GDT_KERNEL_CS_OFFSET, IDT_FLAG_PRESENT | IDT_FLAG_RING3 | IDT_FLAG_INT_GATE);
    set_idt_gate(18, (uint32_t)isr18, GDT_KERNEL_CS_OFFSET, IDT_FLAG_PRESENT | IDT_FLAG_RING3 | IDT_FLAG_INT_GATE);
    set_idt_gate(19, (uint32_t)isr19, GDT_KERNEL_CS_OFFSET, IDT_FLAG_PRESENT | IDT_FLAG_RING3 | IDT_FLAG_INT_GATE);
    set_idt_gate(20, (uint32_t)isr20, GDT_KERNEL_CS_OFFSET, IDT_FLAG_PRESENT | IDT_FLAG_RING3 | IDT_FLAG_INT_GATE);
    set_idt_gate(21, (uint32_t)isr21, GDT_KERNEL_CS_OFFSET, IDT_FLAG_PRESENT | IDT_FLAG_RING3 | IDT_FLAG_INT_GATE);
    set_idt_gate(22, (uint32_t)isr22, GDT_KERNEL_CS_OFFSET, IDT_FLAG_PRESENT | IDT_FLAG_RING3 | IDT_FLAG_INT_GATE);
    set_idt_gate(23, (uint32_t)isr23, GDT_KERNEL_CS_OFFSET, IDT_FLAG_PRESENT | IDT_FLAG_RING3 | IDT_FLAG_INT_GATE);
    set_idt_gate(24, (uint32_t)isr24, GDT_KERNEL_CS_OFFSET, IDT_FLAG_PRESENT | IDT_FLAG_RING3 | IDT_FLAG_INT_GATE);
    set_idt_gate(25, (uint32_t)isr25, GDT_KERNEL_CS_OFFSET, IDT_FLAG_PRESENT | IDT_FLAG_RING3 | IDT_FLAG_INT_GATE);
    set_idt_gate(26, (uint32_t)isr26, GDT_KERNEL_CS_OFFSET, IDT_FLAG_PRESENT | IDT_FLAG_RING3 | IDT_FLAG_INT_GATE);
    set_idt_gate(27, (uint32_t)isr27, GDT_KERNEL_CS_OFFSET, IDT_FLAG_PRESENT | IDT_FLAG_RING3 | IDT_FLAG_INT_GATE);
    set_idt_gate(28, (uint32_t)isr28, GDT_KERNEL_CS_OFFSET, IDT_FLAG_PRESENT | IDT_FLAG_RING3 | IDT_FLAG_INT_GATE);
    set_idt_gate(29, (uint32_t)isr29, GDT_KERNEL_CS_OFFSET, IDT_FLAG_PRESENT | IDT_FLAG_RING3 | IDT_FLAG_INT_GATE);
    set_idt_gate(30, (uint32_t)isr30, GDT_KERNEL_CS_OFFSET, IDT_FLAG_PRESENT | IDT_FLAG_RING3 | IDT_FLAG_INT_GATE);
    set_idt_gate(31, (uint32_t)isr31, GDT_KERNEL_CS_OFFSET, IDT_FLAG_PRESENT | IDT_FLAG_RING3 | IDT_FLAG_INT_GATE);

    // --- Hardware Interrupt Gates (IRQs 32-47) ---
    set_idt_gate(32, (uint32_t)irq0, GDT_KERNEL_CS_OFFSET, IDT_FLAG_PRESENT | IDT_FLAG_RING3 | IDT_FLAG_INT_GATE);
    set_idt_gate(33, (uint32_t)irq1, GDT_KERNEL_CS_OFFSET, IDT_FLAG_PRESENT | IDT_FLAG_RING3 | IDT_FLAG_INT_GATE);
    set_idt_gate(34, (uint32_t)irq2, GDT_KERNEL_CS_OFFSET, IDT_FLAG_PRESENT | IDT_FLAG_RING3 | IDT_FLAG_INT_GATE);
    set_idt_gate(35, (uint32_t)irq3, GDT_KERNEL_CS_OFFSET, IDT_FLAG_PRESENT | IDT_FLAG_RING3 | IDT_FLAG_INT_GATE);
    set_idt_gate(36, (uint32_t)irq4, GDT_KERNEL_CS_OFFSET, IDT_FLAG_PRESENT | IDT_FLAG_RING3 | IDT_FLAG_INT_GATE);
    set_idt_gate(37, (uint32_t)irq5, GDT_KERNEL_CS_OFFSET, IDT_FLAG_PRESENT | IDT_FLAG_RING3 | IDT_FLAG_INT_GATE);
    set_idt_gate(38, (uint32_t)irq6, GDT_KERNEL_CS_OFFSET, IDT_FLAG_PRESENT | IDT_FLAG_RING3 | IDT_FLAG_INT_GATE);
    set_idt_gate(39, (uint32_t)irq7, GDT_KERNEL_CS_OFFSET, IDT_FLAG_PRESENT | IDT_FLAG_RING3 | IDT_FLAG_INT_GATE);
    set_idt_gate(40, (uint32_t)irq8, GDT_KERNEL_CS_OFFSET, IDT_FLAG_PRESENT | IDT_FLAG_RING3 | IDT_FLAG_INT_GATE);
    set_idt_gate(41, (uint32_t)irq9, GDT_KERNEL_CS_OFFSET, IDT_FLAG_PRESENT | IDT_FLAG_RING3 | IDT_FLAG_INT_GATE);
    set_idt_gate(42, (uint32_t)irq10, GDT_KERNEL_CS_OFFSET, IDT_FLAG_PRESENT | IDT_FLAG_RING3 | IDT_FLAG_INT_GATE);
    set_idt_gate(43, (uint32_t)irq11, GDT_KERNEL_CS_OFFSET, IDT_FLAG_PRESENT | IDT_FLAG_RING3 | IDT_FLAG_INT_GATE);
    set_idt_gate(44, (uint32_t)irq12, GDT_KERNEL_CS_OFFSET, IDT_FLAG_PRESENT | IDT_FLAG_RING3 | IDT_FLAG_INT_GATE);
    set_idt_gate(45, (uint32_t)irq13, GDT_KERNEL_CS_OFFSET, IDT_FLAG_PRESENT | IDT_FLAG_RING3 | IDT_FLAG_INT_GATE);
    set_idt_gate(46, (uint32_t)irq14, GDT_KERNEL_CS_OFFSET, IDT_FLAG_PRESENT | IDT_FLAG_RING3 | IDT_FLAG_INT_GATE);
    set_idt_gate(47, (uint32_t)irq15, GDT_KERNEL_CS_OFFSET, IDT_FLAG_PRESENT | IDT_FLAG_RING3 | IDT_FLAG_INT_GATE);

    // --- System Call Gates ---
    set_idt_gate(128, (uint32_t)isr128, GDT_KERNEL_CS_OFFSET, IDT_FLAG_PRESENT | IDT_FLAG_RING3 | IDT_FLAG_INT_GATE);
    set_idt_gate(177, (uint32_t)isr177, GDT_KERNEL_CS_OFFSET, IDT_FLAG_PRESENT | IDT_FLAG_RING3 | IDT_FLAG_INT_GATE);

    // 5. Finally, load the IDT using the ASM command 'lidt'
    idt_flush((uint32_t)&idtr);
}

// Human-readable strings describing the first 32 exceptions
const char *exception_messages[] = {
    [0] = "Division By Zero",
    [1] = "Debug",
    [2] = "Non Maskable Interrupt",
    [3] = "Breakpoint",
    [4] = "Into Detected Overflow",
    [5] = "Out of Bounds",
    [6] = "Invalid Opcode",
    [7] = "No Coprocessor",
    [8] = "Double fault",
    [9] = "Coprocessor Segment Overrun",
    [10] = "Bad TSS",
    [11] = "Segment not present",
    [12] = "Stack fault",
    [13] = "General protection fault",
    [14] = "Page fault",
    [15] = "Unknown Interrupt",
    [16] = "Coprocessor Fault",
    [17] = "Alignment Fault",
    [18] = "Machine Check",
    [19] = "Reserved",
    [20] = "Reserved",
    [21] = "Reserved",
    [22] = "Reserved",
    [23] = "Reserved",
    [24] = "Reserved",
    [25] = "Reserved",
    [26] = "Reserved",
    [27] = "Reserved",
    [28] = "Reserved",
    [29] = "Reserved",
    [30] = "Reserved",
    [31] = "Reserved"};

/*
 * ISR Handler
 * This is the main handler for CPU Exceptions (0-31).
 * It is called by the assembly stub when a fault occurs.
 */
void isr_handler(InteruptReg *regs)
{
    // If it is a known CPU exception
    if (regs->intr_num < 32)
    {
        terminal_writestring(exception_messages[regs->intr_num]);
        terminal_writestring("\nException! System Halted\n");

        // Hang the system to prevent further damage
        for (;;);
    }
}

// Array of function pointers to handle custom IRQ handlers (e.g., keyboard_handler)
void *irq_routines[16] = {0};

/*
 * Install IRQ Handler
 * Allows other parts of the kernel (like the keyboard driver) to register a function
 * to be called when a specific IRQ fires.
 */
void irq_install_handler(int irq, void (*handler)(InteruptReg *r))
{
    irq_routines[irq] = handler;
}

/*
 * Uninstall IRQ Handler
 * Removes the custom handler for an IRQ.
 */
void irq_uninstall_handler(int irq)
{
    irq_routines[irq] = NULL;
}

/*
 * IRQ Handler
 * This is the main handler for Hardware Interrupts (32-47).
 * It finds the registered handler function and manages the PIC.
 */
void irq_handler(InteruptReg *regs)
{
    // Function pointer for the specific handler
    void (*handler)(InteruptReg *regs);

    // Map interrupt number back to IRQ number (0-15)
    // Example: Interrupt 33 is IRQ 1 (Keyboard)
    handler = irq_routines[regs->intr_num - 32];

    // If a custom handler exists, execute it
    if (handler)
    {
        handler(regs);
    }

    // Send End of Interrupt (EOI) command to the PICs.
    // If the IRQ came from the Slave PIC (IRQ 8-15), we must send EOI to both.
    if (regs->intr_num >= 40)
    {
        out_port_b(0xA0, 0x20); // EOI to Slave PIC
    }

    // Always send EOI to Master PIC
    out_port_b(0x20, 0x20);
}
