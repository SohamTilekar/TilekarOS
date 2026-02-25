[bits 32]

section .text

global context_switch

; context_switch(uint32_t* current_esp, uint32_t next_esp)
context_switch:
    ; Save callee-saved registers
    push ebp
    push ebx
    push esi
    push edi

    ; Get current_esp_ptr (arg1)
    ; Stack layout now:
    ; [esp + 0]  = edi
    ; [esp + 4]  = esi
    ; [esp + 8]  = ebx
    ; [esp + 12] = ebp
    ; [esp + 16] = return address
    ; [esp + 20] = current_esp_ptr
    ; [esp + 24] = next_esp

    mov eax, [esp + 20]
    mov [eax], esp      ; Save current ESP

    mov esp, [esp + 24] ; Load next ESP

    ; Restore callee-saved registers
    pop edi
    pop esi
    pop ebx
    pop ebp

    ret
