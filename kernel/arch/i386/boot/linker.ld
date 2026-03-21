/* The bootloader will look at this image and start execution at the symbol
   designated as the entry point. */
ENTRY(_start)

/* Tell the linker that we want the specific sections in the output file
   to be organized as follows: */
SECTIONS
{
    /* --- PHYSICAL MEMORY SETUP --- */

    /* The kernel will be loaded at 1MB into physical memory. */
    /* We skip the first 1MB to avoid BIOS routines, the VGA text buffer (0xB8000),
       and other hardware-reserved areas typically found in the lower 1MB. */
    . = 1M;

    /* The Multiboot header must appear very early in the binary (within the
       first 8 KiB) so the bootloader (like GRUB) can find it. */
    .multiboot :
    {
        /* Ensure the multiboot header stays at the beginning */
        *(.multiboot)
    }

    /* Sometimes the compiler adds a build-id note. We include it explicitly
       to prevent the linker from placing it somewhere that breaks the layout. */
    .note.gnu.build-id ALIGN(4K) :
    {
        *(.note.gnu.build-id)
    }

    /* The .boot section contains the "trampoline" code.
       Since paging is not yet enabled when the kernel starts, this code
       must execute from low physical memory (around 1MB). */
    .boot ALIGN(4K) :
    {
        *(.boot)
    }

    /* --- HIGHER HALF KERNEL SETUP --- */

    /* We now increment the location counter (.) by 3GB (0xC0000000).
       From this point on, all symbols will be assigned virtual addresses
       in the upper half of memory (e.g., 0xC0100000), but we still need
       to load them at physical address 0x00100000. */
    . += 0xC0000000;

    /* CODE SECTION (.text)
       ALIGN(4K): Align sections on 4KB page boundaries for easy mapping.
       AT(...):   This specifies the Load Memory Address (LMA).
                  We calculate the physical address by taking the current
                  Virtual Address (ADDR(.text)) and subtracting the 3GB offset. */
    .text ALIGN(4K) : AT(ADDR(.text) - 0xC0000000)
    {
        *(.text)
    }

    /* READ-ONLY DATA (.rodata)
       Contains constants, string literals, and C++ vtables. */
    .rodata ALIGN(4K) : AT(ADDR(.rodata) - 0xC0000000)
    {
        *(.rodata)
    }

    /* INITIALIZED DATA (.data)
       Contains global variables that have been assigned a value (e.g., int x = 5;). */
    .data ALIGN(4K) : AT(ADDR(.data) - 0xC0000000)
    {
        *(.data)
    }

    /* UNINITIALIZED DATA (.bss)
       Contains global variables that are zeroed or uninitialized (e.g., int x;).
       Also usually contains the kernel stack.
       *(COMMON) catches C variables that are "tentative" definitions. */
    .bss ALIGN(4K) : AT(ADDR(.bss) - 0xC0000000)
    {
        *(COMMON)
        *(.bss)
    }

    /* End of the kernel image marker. Useful for the memory manager (PMM)
       to know where free memory begins. */
    _kernel_end = .;
}
