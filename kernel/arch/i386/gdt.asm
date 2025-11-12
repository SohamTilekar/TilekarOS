section .text

; void GDT_Load(GDTDescriptor* descriptor);
global GDT_Load
GDT_Load:

    mov eax, [esp + 4]
    lgdt [eax]

    ; reload data segment registers
    mov eax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    ; Reload code selector register
    jmp 0x08:.flush ; 0x08 is a stand-in for code segment
.flush:
    ret ; retrun cz dont forget that GDT_Load is a function

global TSS_Load
TSS_Load:
    mov ax, 0x28 ; offset of the TSS
    ltr ax
    ret
