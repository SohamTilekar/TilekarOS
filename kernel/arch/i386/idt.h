#ifndef IDT_H
#define IDT_H

#include <stdint.h>

/*
 * Interrupt Registers
 * A snapshot of the CPU state pushed onto the stack by the assembly stubs
 * before calling the C handler. Used to analyze crashes or handle inputs.
 */
typedef struct
{
    uint32_t cr2;
    uint32_t ds;                                     // Data segment selector
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pushed by pusha
    uint32_t intr_num, err_cod;                      // Interrupt number and error code (if applicable)
    uint32_t eip, csm, eflags, useresp, ss;          // Pushed by the processor automatically

} InteruptReg;

// Helper function to write to IO ports (defined in assembly)
void out_port_b(uint16_t port, uint8_t value);

// IDT Initialization
void init_idt();

// IRQ Handling
void irq_install_handler(int irq, void (*handler)(InteruptReg *r));
void irq_uninstall_handler(int irq);

#endif
