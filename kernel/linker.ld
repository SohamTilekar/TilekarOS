ENTRY(_start)

SECTIONS
{
	/* Kernel load address.
	   Using 2 MiB instead of the legacy 1 MiB offset ensures compatibility
	   with both BIOS and UEFI boot paths. The image is non-relocatable,
	   so all linking assumes a fixed physical and virtual base at 2 MiB. */
	. = 2M;

	/* Code layout.
	   Multiboot header must appear within the first 8 KiB, per spec.
	   Followed immediately by executable text sections. */
	.text BLOCK(4K) : ALIGN(4K)
	{
		*(.multiboot)
		*(.text)
	}

	/* Read-only data: constants, string literals, vtables, etc. */
	.rodata BLOCK(4K) : ALIGN(4K)
	{
		*(.rodata)
	}

	/* Writable, pre-initialized data. */
	.data BLOCK(4K) : ALIGN(4K)
	{
		*(.data)
	}

	/* Zero-initialized data and kernel stack.
	   Includes legacy COMMON symbols for older toolchains. */
	.bss BLOCK(4K) : ALIGN(4K)
	{
		*(COMMON)
		*(.bss)
	}

	/* Catch-all for unassigned sections.
	   Extend with explicit mappings when adding new segment types. */
}
