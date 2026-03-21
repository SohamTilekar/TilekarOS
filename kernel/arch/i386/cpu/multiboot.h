#include <stdint.h>

/* The magic number for the Multiboot header. */
#define MULTIBOOT_HEADER_MAGIC                  0x1BADB002

/* The magic number passed by a Multiboot-compliant bootloader. */
#define MULTIBOOT_BOOTLOADER_MAGIC              0x2BADB002

/* Flags set in the Multiboot header 'flags' field. */
#define MULTIBOOT_HEADER_PAGE_ALIGN             0x00000001
#define MULTIBOOT_HEADER_MEMORY_INFO            0x00000002
#define MULTIBOOT_HEADER_VIDEO_MODE             0x00000004

/* Flags set in the 'flags' member of the MultiBootInfo structure. */
#define MULTIBOOT_INFO_MEMORY                   0x00000001
#define MULTIBOOT_INFO_BOOTDEV                  0x00000002
#define MULTIBOOT_INFO_CMDLINE                  0x00000004
#define MULTIBOOT_INFO_MODS                     0x00000008
#define MULTIBOOT_INFO_AOUT_SYMS                0x00000010
#define MULTIBOOT_INFO_ELF_SHDR                 0x00000020
#define MULTIBOOT_INFO_MEM_MAP                  0x00000040
#define MULTIBOOT_INFO_DRIVE_INFO               0x00000080
#define MULTIBOOT_INFO_CONFIG_TABLE             0x00000100
#define MULTIBOOT_INFO_BOOT_LOADER_NAME         0x00000200
#define MULTIBOOT_INFO_APM_TABLE                0x00000400
#define MULTIBOOT_INFO_VBE_INFO                 0x00000800
#define MULTIBOOT_INFO_FRAMEBUFFER_INFO         0x00001000

typedef struct
{
  uint32_t tabsize;
  uint32_t strsize;
  uint32_t addr;
  uint32_t reserved;
} MultiBootAoutSymbolTable;

typedef struct
{
  uint32_t num;
  uint32_t size;
  uint32_t addr;
  uint32_t shndx;
} MultiBootElfSectionHeaderTable;

typedef struct
{
  uint32_t flags;
  uint32_t mem_lower;
  uint32_t mem_upper;
  uint32_t boot_device;

  uint32_t cmdline;
  uint32_t mods_count;
  uint32_t mods_addr;

  union
  {
    MultiBootAoutSymbolTable aout_sym;
    MultiBootElfSectionHeaderTable elf_sec;
  } u;

  uint32_t mmap_length;
  uint32_t mmap_addr;

  uint32_t drives_length;
  uint32_t drives_addr;

  uint32_t config_table;
  uint32_t boot_loader_name;

  uint32_t apm_table;

  uint32_t vbe_control_info;
  uint32_t vbe_mode_info;
  uint16_t vbe_mode;
  uint16_t vbe_interface_seg;
  uint16_t vbe_interface_off;
  uint16_t vbe_interface_len;
} MultiBootInfo;

typedef enum
{
  MULTIBOOT_MEMORY_AVAILABLE              = 1,
  MULTIBOOT_MEMORY_RESERVED               = 2,
  MULTIBOOT_MEMORY_ACPI_RECLAIMABLE       = 3,
  MULTIBOOT_MEMORY_NVS                    = 4,
  MULTIBOOT_MEMORY_BADRAM                 = 5,
} MultiBootMemoryType;

typedef struct
{
  uint32_t size;
  uint32_t addr_low;
  uint32_t addr_high;
  uint32_t len_low;
  uint32_t len_high;
  uint32_t type;
} __attribute__((packed)) MultiBootMmapEntry;
