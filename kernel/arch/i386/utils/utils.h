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
    uint32_t gs, fs, es, ds;                         // Pushed by segment registers
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pushed by pushad
    uint32_t intr_num, err_code;                     // Interrupt number and error code (if applicable)
    uint32_t eip, cs, eflags, useresp, ss;           // Pushed by the processor automatically
} InterruptReg_t;

/**
 * out_port_b - Writes a byte to an I/O port.
 * @port: The I/O port address.
 * @value: The value to write.
 */
void out_port_b(uint16_t port, uint8_t value);

/**
 * out_port_w - Writes a word to an I/O port.
 * @port: The I/O port address.
 * @value: The value to write.
 */
void out_port_w(uint16_t port, uint16_t value);

/**
 * out_port_l - Writes a double word (32-bit) to an I/O port.
 * @port: The I/O port address.
 * @value: The value to write.
 */
void out_port_l(uint16_t port, uint32_t value);

/**
 * in_port_b - Reads a byte from an I/O port.
 * @port: The I/O port address.
 *
 * Return: The byte read from the port.
 */
uint8_t in_port_b(uint16_t port);

/**
 * in_port_w - Reads a word from an I/O port.
 * @port: The I/O port address.
 *
 * Return: The word read from the port.
 */
uint16_t in_port_w(uint16_t port);

/**
 * in_port_l - Reads a double word (32-bit) from an I/O port.
 * @port: The I/O port address.
 *
 * Return: The dword read from the port.
 */
uint32_t in_port_l(uint16_t port);

#define CEIL_DIV(a,b) (((a + b) - 1)/b)

/**
 * interrupt_save - Saves the current EFLAGS and disables interrupts.
 * Return: The saved EFLAGS value.
 */
static inline uint32_t interrupt_save(void) {
    uint32_t flags;
    asm volatile("pushfl\n\tpopl %0\n\tcli" : "=r"(flags));
    return flags;
}

#include <stdbool.h>

/**
 * interrupt_restore - Restores the EFLAGS (and thus interrupt state).
 * @flags: The EFLAGS value to restore.
 */
static inline void interrupt_restore(uint32_t flags) {
    asm volatile("pushl %0\n\tpopfl" : : "r"(flags) : "cc", "memory");
}

#endif // ARCH_I386_UTILS_H
