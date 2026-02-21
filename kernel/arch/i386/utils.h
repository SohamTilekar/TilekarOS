#ifndef ARCH_I386_UTILS_H
#define ARCH_I386_UTILS_H

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

/**
 * out_port_b - Writes a byte to an I/O port.
 * @port: The I/O port address.
 * @value: The value to write.
 */
void out_port_b(uint16_t port, uint8_t value);

/**
 * in_port_b - Reads a byte from an I/O port.
 * @port: The I/O port address.
 *
 * Return: The byte read from the port.
 */
uint8_t in_port_b(uint16_t port);

#define CEIL_DIV(a,b) (((a + b) - 1)/b)

#endif // ARCH_I386_UTILS_H
