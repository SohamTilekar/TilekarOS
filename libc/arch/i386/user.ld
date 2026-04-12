ENTRY(_start)

SECTIONS
{
    /* Standard userspace load address */
    . = 0x400000;

    .text :
    {
        *(.text*)
    }

    .rodata :
    {
        *(.rodata*)
    }

    .data :
    {
        *(.data*)
    }

    .bss :
    {
        *(.bss*)
        *(COMMON)
    }

    /* Discard comments and other unnecessary sections */
    /DISCARD/ :
    {
        *(.comment)
        *(.note*)
    }
}
