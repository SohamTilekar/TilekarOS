; linked with the C code in 'gdt.c'.
section .text

; -- `void gdt_load_register(GDTDescriptor* descriptor)` --
; Loads the GDT descriptor into the processor's GDTR register
; and reloads all segment registers to use the new GDT.
;
global gdt_load_register
gdt_load_register:
    ; Get the argument (gdt_descriptor_t* descriptor) from the stack
    mov eax, [esp + 4]

    ; Get the argument (gdt_descriptor_t* descriptor) from the stack
    lgdt [eax]

    ; We must reload all segment registers to use the new GDT.
    ; We load the Kernel Data Segment selector into all data segments.
    mov eax, GDT_KERNEL_DS_OFFSET
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Reload the code segment register (cs) using a far jump.
    ; This flushes the CPU's pre-fetch queue and forces it
    ; to use the new code segment.
    jmp GDT_KERNEL_CS_OFFSET:.flush ; GDT_KERNEL_CS_OFFSET is a stand-in for code segment
.flush:
    ret ; retrun cz dont forget that gdt_load_register is a function

; -- `void tss_load_register(void)` --
; Loads the Task Register (ltr) with the selector for our TSS.
;
global tss_load_register
tss_load_register:
    mov ax, GDT_TSS_OFFSET ; offset of the TSS
    ltr ax
    ret
