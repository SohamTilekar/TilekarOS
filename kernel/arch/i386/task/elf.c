#include "elf.h"
#include "memory.h"
#include <string.h>
#include <stdio.h>

#define ELF_MAGIC 0x464C457F

static uint32_t align_up_page(uint32_t value) {
    return (value + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
}

bool elf_check_supported(Elf32_Ehdr *hdr) {
    if (!hdr) return false;
    if (*(uint32_t*)hdr->e_ident != ELF_MAGIC) return false;
    if (hdr->e_ident[EI_CLASS] != ELFCLASS32) return false;
    if (hdr->e_ident[EI_DATA] != ELFDATA2LSB) return false;
    if (hdr->e_machine != EM_386) return false;
    if (hdr->e_type != ET_EXEC) return false;
    return true;
}

void* elf_load_segments(Elf32_Ehdr *hdr, void* elf_data, uint8_t privilege_level, uint32_t* heap_start_out) {
    Elf32_Phdr *ph = (Elf32_Phdr *)((uint8_t *)elf_data + hdr->e_phoff);
    uint32_t max_mapped_end = 0;

    for (int i = 0; i < hdr->e_phnum; i++) {
        if (ph[i].p_type == PT_LOAD) {
            uint32_t vaddr = ph[i].p_vaddr;
            uint32_t filesz = ph[i].p_filesz;
            uint32_t memsz = ph[i].p_memsz;
            uint32_t offset = ph[i].p_offset;

            if (memsz == 0) {
                continue;
            }

            uint32_t num_pages = (memsz + (vaddr & 0xFFF) + PAGE_SIZE - 1) / PAGE_SIZE;
            uint32_t start_vaddr = vaddr & ~0xFFF;
            uint32_t segment_end = vaddr + memsz;
            if (segment_end > max_mapped_end) {
                max_mapped_end = segment_end;
            }

            for (uint32_t j = 0; j < num_pages; j++) {
                uint32_t page_vaddr = start_vaddr + j * PAGE_SIZE;
                uint32_t phys = pmm_alloc_page_frame();
                if (!phys) return NULL;
                
                uint32_t flags = PAGE_FLAG_PRESENT;
                if (privilege_level == 3) flags |= PAGE_FLAG_USER;
                if (ph[i].p_flags & PF_W) flags |= PAGE_FLAG_WRITE;
                
                memory_map_page(page_vaddr, phys, flags);
            }

            // Copy data
            memcpy((void *)vaddr, (uint8_t *)elf_data + offset, filesz);
            
            // Zero out remaining memory (BSS)
            if (memsz > filesz) {
                memset((void *)(vaddr + filesz), 0, memsz - filesz);
            }
        }
    }

    if (heap_start_out) {
        uint32_t base = max_mapped_end ? max_mapped_end : (uint32_t)hdr->e_entry;
        *heap_start_out = align_up_page(base);
    }

    return (void *)hdr->e_entry;
}
