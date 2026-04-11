#ifndef ARCH_I386_ELF_H
#define ARCH_I386_ELF_H

#include <stdint.h>
#include <stdbool.h>

#define ELF_MAGIC 0x464C457F // "\x7FELF" in little endian

typedef uint32_t Elf32_Addr;
typedef uint16_t Elf32_Half;
typedef uint32_t Elf32_Off;
typedef int32_t  Elf32_Sword;
typedef uint32_t Elf32_Word;

#define EI_NIDENT 16

typedef struct {
    unsigned char e_ident[EI_NIDENT];
    Elf32_Half    e_type;
    Elf32_Half    e_machine;
    Elf32_Word    e_version;
    Elf32_Addr    e_entry;
    Elf32_Off     e_phoff;
    Elf32_Off     e_shoff;
    Elf32_Word    e_flags;
    Elf32_Half    e_ehsize;
    Elf32_Half    e_phentsize;
    Elf32_Half    e_phnum;
    Elf32_Half    e_shentsize;
    Elf32_Half    e_shnum;
    Elf32_Half    e_shstrndx;
} Elf32_Ehdr;

enum Elf_Ident {
    EI_MAG0 = 0, // 0x7F
    EI_MAG1 = 1, // 'E'
    EI_MAG2 = 2, // 'L'
    EI_MAG3 = 3, // 'F'
    EI_CLASS = 4,
    EI_DATA = 5,
    EI_VERSION = 6,
    EI_OSABI = 7,
    EI_ABIVERSION = 8,
    EI_PAD = 9
};

#define ELFCLASS32 1
#define ELFDATA2LSB 1

#define ET_EXEC 2
#define EM_386 3

typedef struct {
    Elf32_Word p_type;
    Elf32_Off  p_offset;
    Elf32_Addr p_vaddr;
    Elf32_Addr p_paddr;
    Elf32_Word p_filesz;
    Elf32_Word p_memsz;
    Elf32_Word p_flags;
    Elf32_Word p_align;
} Elf32_Phdr;

#define PT_NULL    0
#define PT_LOAD    1
#define PT_DYNAMIC 2
#define PT_INTERP  3
#define PT_NOTE    4
#define PT_SHLIB   5
#define PT_PHDR    6

#define PF_X 1
#define PF_W 2
#define PF_R 4

bool elf_check_supported(Elf32_Ehdr *hdr);
void* elf_load_segments(Elf32_Ehdr *hdr, void* elf_data, uint8_t privilege_level);

#endif // ARCH_I386_ELF_H
