[bits 32]
section .rodata

global _start_elf_user_task
global _end_elf_user_task

_start_elf_user_task:
    incbin "../../kernel/user_task.elf"
_end_elf_user_task:
