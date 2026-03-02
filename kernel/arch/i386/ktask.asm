[bits 32]

struc registers_t
    .gs:      resd 1
    .fs:      resd 1
    .es:      resd 1
    .ds:      resd 1
    .edi:     resd 1
    .esi:     resd 1
    .ebp:     resd 1
    .esp:     resd 1
    .ebx:     resd 1
    .edx:     resd 1
    .ecx:     resd 1
    .eax:     resd 1
    .eip:     resd 1
    .cs:      resd 1
    .eflags:  resd 1
endstruc

section .text

global context_switch

; context_switch(uint32_t** current_esp, uint32_t* next_esp, uint32_t next_cr3, uint32_t intr_num)
context_switch:
    ; The stack initially has the return EIP at [esp].
    ; We need to simulate an interrupt frame so we can return via iret.
    ; Pop the return EIP so we can push it properly with CS and EFLAGS.
    pop eax            ; EAX = Return EIP

    ; Push standard interrupt frame (EFLAGS, CS, EIP)
    pushfd             ; Push EFLAGS
    push cs            ; Push CS
    push eax           ; Push EIP

    ; Save general purpose registers (pushad)
    pushad

    ; Save segment registers
    push ds
    push es
    push fs
    push gs

    ; Stack layout analysis:
    ; Before pop eax:
    ;   [esp+0] = Return EIP
    ;   [esp+4] = current_esp (arg1)
    ;   [esp+8] = next_esp (arg2)
    ;   [esp+12] = next_cr3 (arg3)
    ;   [esp+16] = intr_num (arg4)
    ; After pop eax:
    ;   [esp+0] = current_esp (arg1)
    ;   [esp+4] = next_esp (arg2)
    ;   [esp+8] = next_cr3 (arg3)
    ;   [esp+12] = intr_num (arg4)
    ; After pushing 15 dwords (60 bytes) - EFLAGS(1), CS(1), EIP(1), pushad(8), segs(4):
    ;   [esp+registers_t_size] = current_esp (arg1)
    ;   [esp+registers_t_size+4] = next_esp (arg2)
    ;   [esp+registers_t_size+8] = next_cr3 (arg3)
    ;   [esp+registers_t_size+12] = intr_num (arg4)

    mov eax, [esp + registers_t_size] ; current_esp pointer (arg1)
    
    ; If the current_esp pointer is NULL (dummy), don't save ESP
    test eax, eax
    jz .skip_save
    mov [eax], esp      ; Save current ESP into current task's kernel_stack
.skip_save:

    ; Load next CR3
    mov edx, [esp + registers_t_size + 8] ; next_cr3 (arg3)
    
    ; Read current cr3 to see if we actually need to change it
    ; Avoiding unnecessary CR3 loads prevents TLB flushes
    mov eax, cr3
    cmp eax, edx
    je .skip_cr3_load
    mov cr3, edx        ; Load new page directory
.skip_cr3_load:

    ; Read intr_num (arg4) before switching the stack pointer
    mov ebx, [esp + registers_t_size + 12]

    ; Load next task's kernel_stack into ESP
    mov esp, [esp + registers_t_size + 4] 

    ; Send EOI if intr_num >= 32
    cmp ebx, 32
    jl .no_eoi
    
    ; We are safe to call pic_send_eoi because popad hasn't happened yet!
    ; Any clobbered EAX, ECX, EDX will be safely overwritten by the upcoming popad.
    push ebx ; Argument intr_num
    extern pic_send_eoi
    call pic_send_eoi
    add esp, 4

.no_eoi:

    ; Restore segment registers for the next task
    pop gs
    pop fs
    pop es
    pop ds

    ; Restore general purpose registers for the next task
    popad

    ; Return to the next task using iret!
    ; This will pop EIP, CS, and EFLAGS from the next task's stack.
    iret
