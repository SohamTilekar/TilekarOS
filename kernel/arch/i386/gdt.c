#include "local_config.h"
#include <stdint.h>

/**
 * @brief single 8-byte GDT entry.
 *
 * https://wiki.osdev.org/Global_Descriptor_Table
 */
typedef struct GDTEntry {
  uint16_t limit_low;     // The lower 16 bits of the limit.
  uint16_t base_low;      // The lower 16 bits of the base.
  uint8_t base_middle;    // The next 8 bits of the base (16-23).
  uint8_t access;         // Access flags (see gdt_access_t).
  uint8_t flags_limit_hi; // Granularity, 32/16-bit, and high 4 bits of limit
                          // (16-19).
  uint8_t base_high;      // The last 8 bits of the base (24-31).
} __attribute__((packed)) GDTEntry;

/**
 * @brief 32-bit Task State Segment (TSS)
 *
 * Official Intel format (Vol. 3, Table 7-2).
 * Used for privilege-level stack switching and hardware task management.
 *
 * Each 16-bit field followed by a reserved 16-bit field ensures exact
 * alignment to the hardware-defined byte offsets (_resX).

 * https://wiki.osdev.org/Task_State_Segment
 */
typedef struct __attribute__((packed)) {
  uint16_t backlink, _res0;      // Previous Task Link (if hardware task switch)
  uint32_t esp0;                 // Stack Pointer (Ring 0)
  uint16_t ss0, _res1;           // Stack Segment (Ring 0)
  uint32_t esp1;                 // Stack Pointer (Ring 1)
  uint16_t ss1, _res2;           // Stack Segment (Ring 1)
  uint32_t esp2;                 // Stack Pointer (Ring 2)
  uint16_t ss2, _res3;           // Stack Segment (Ring 2)
  uint32_t cr3;                  // Page Directory Base Register
  uint32_t eip;                  // Instruction Pointer
  uint32_t eflags;               // Flags Register
  uint32_t eax, ecx, edx, ebx;   // General-Purpose Register
  uint32_t esp;                  // Stack Pointer
  uint32_t ebp;                  // Base Pointer
  uint32_t esi;                  // Source Index
  uint32_t edi;                  // Destination Index
  uint16_t es, _res4;            // Extra Segment Selector
  uint16_t cs, _res5;            // Code Segment Selector
  uint16_t ss, _res6;            // Stack Segment Selector
  uint16_t ds, _res7;            // Data Segment Selector
  uint16_t fs, _res8;            // FS Segment Selector
  uint16_t gs, _res9;            // GS Segment Selector
  uint16_t ldt_selector, _res10; // Local Descriptor Table Selector
  uint16_t t, iomap_base;        // Trap flag and I/O Map Base Address
} TSSEntry;

/**
 * @brief Represents the GDT Descriptor (GDTR payload).
 *
 * This structure is loaded into the GDTR register using the `lgdt` instruction.
 *
 * https://wiki.osdev.org/Global_Descriptor_Table#GDTR
 */
typedef struct {
  uint16_t limit;    // sizeof(GDT) - 1
  GDTEntry *address; // Linear address of the GDT.
} __attribute__((packed)) GDTDescriptor;

typedef enum {
  // --- Segment Type (bits 0-3) + S bit (bit 4) ---
  // System Segments (S=0)
  GDT_TYPE_TSS_32_AVAIL = 0x09, // 32-bit TSS (Available)
  GDT_TYPE_TSS_32_BUSY = 0x09,  // 32-bit TSS (Busy)
  GDT_TYPE_TSS_16_AVAIL = 0x09, // 32-bit TSS (Available)
  GDT_TYPE_TSS_16_BUSY = 0x09,  // 32-bit TSS (Busy)
  GDT_TYPE_LDT = 0x2,
  GDT_ACCESS_DESCRIPTOR_TSS = 0x00,

  GDT_ACCESS_CODE_READABLE = 0x02,
  GDT_ACCESS_DATA_WRITEABLE = 0x02,

  GDT_ACCESS_CODE_CONFORMING = 0x04,
  GDT_ACCESS_DATA_DIRECTION_NORMAL = 0x00,
  GDT_ACCESS_DATA_DIRECTION_DOWN = 0x04,

  GDT_ACCESS_DATA_SEGMENT = 0x10,
  GDT_ACCESS_CODE_SEGMENT = 0x18,

  // Privilege Level (DPL, bits 5-6)
  GDT_ACCESS_RING0 = 0x00,
  GDT_ACCESS_RING1 = 0x20,
  GDT_ACCESS_RING2 = 0x40,
  GDT_ACCESS_RING3 = 0x60,

  // Present (P, bit 7)
  GDT_ACCESS_PRESENT = 0x80,

} GDT_ACCESS;

typedef enum {
  // Size (D/B bit, bit 2)
  GDT_FLAG_64BIT = 0x20,
  GDT_FLAG_32BIT = 0x40,
  GDT_FLAG_16BIT = 0x00,

  // Granularity (G bit, bit 3)
  GDT_FLAG_GRANULARITY_1B = 0x00,
  GDT_FLAG_GRANULARITY_4K = 0x80,
} GDT_FLAGS;

// Helper macros
#define GDT_PACK_LIMIT_LOW(limit) ((limit) & 0xFFFF)
#define GDT_PACK_BASE_LOW(base) ((base) & 0xFFFF)
#define GDT_PACK_BASE_MIDDLE(base) (((base) >> 16) & 0xFF)
#define GDT_PACK_FLAGS_LIMIT_HI(limit, flags)                                  \
  ((((limit) >> 16) & 0x0F) | ((flags) & 0xF0))
#define GDT_PACK_BASE_HIGH(base) (((base) >> 24) & 0xFF)

/**
 * @brief Macro to create a GDT_ENTRY initializer.
 * Simplifies the creation of GDT entries at compile time.
 */
#define GDT_ENTRY(base, limit, access, flags)                                  \
  {GDT_PACK_LIMIT_LOW(limit),                                                  \
   GDT_PACK_BASE_LOW(base),                                                    \
   GDT_PACK_BASE_MIDDLE(base),                                                 \
   (access),                                                                   \
   GDT_PACK_FLAGS_LIMIT_HI(limit, flags),                                      \
   GDT_PACK_BASE_HIGH(base)}


/**
 * @brief The Task State Segment (TSS) entry.
 *
 * We only need one for the whole system, to define the Ring 0 stack
 * that the CPU will switch to upon an interrupt or exception.
 * This matches the `static TSSEntry tss_entry` from the original code.
 */
TSSEntry tss_entry = {
    // Set kernel stack segment (GDT_KERNEL_DS_OFFSET = Kernel Data) and pointer.
    // ESP0 will be set by the kernel during context switches or task creation.
    .ss0 = GDT_KERNEL_DS_OFFSET,
    .esp0 = 0x0,

    // Set segment selectors for user mode (RPL=3).
    // GDT_KERNEL_CS_OFFSET = Kernel Code, GDT_KERNEL_DS_OFFSET = Kernel Data. | 0x3 sets RPL to Ring 3.
    .cs = GDT_KERNEL_CS_OFFSET | 0x3,
    .ds = GDT_KERNEL_DS_OFFSET | 0x3,
    .ss = GDT_KERNEL_DS_OFFSET | 0x3,
    .es = GDT_KERNEL_DS_OFFSET | 0x3,
    .fs = GDT_KERNEL_DS_OFFSET | 0x3,
    .gs = GDT_KERNEL_DS_OFFSET | 0x3,
};

GDTEntry gdt[] = {
    // NULL descriptor
    [0] = GDT_ENTRY(0, 0, 0, 0),

    // Kernel 32-bit code segment
    [GDT_KERNEL_CS_INDEX] = GDT_ENTRY(0, 0xFFFFF,
              GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_CODE_SEGMENT |
                  GDT_ACCESS_CODE_READABLE,
              GDT_FLAG_32BIT | GDT_FLAG_GRANULARITY_4K),

    // Kernel 32-bit data segment
    [GDT_KERNEL_DS_INDEX] = GDT_ENTRY(0, 0xFFFFF,
              GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_DATA_SEGMENT |
                  GDT_ACCESS_DATA_WRITEABLE,
              GDT_FLAG_32BIT | GDT_FLAG_GRANULARITY_4K),

    // User 32-bit code segment
    [GDT_USER_CS_INDEX] = GDT_ENTRY(0, 0xFFFFF,
              GDT_ACCESS_PRESENT | GDT_ACCESS_RING3 | GDT_ACCESS_CODE_SEGMENT |
                  GDT_ACCESS_CODE_READABLE,
              GDT_FLAG_32BIT | GDT_FLAG_GRANULARITY_4K),

    // User 32-bit data segment
    [GDT_USER_DS_INDEX] = GDT_ENTRY(0, 0xFFFFF,
              GDT_ACCESS_PRESENT | GDT_ACCESS_RING3 | GDT_ACCESS_DATA_SEGMENT |
                  GDT_ACCESS_DATA_WRITEABLE,
              GDT_FLAG_32BIT | GDT_FLAG_GRANULARITY_4K),

    // Task State Segment
    [GDT_TSS_INDEX] = {/* Set by `writeTSS` in `init_gdt` */},
};

GDTDescriptor gdt_discriptor = {sizeof(gdt) - 1, gdt};

// Assembly functions
// These functions are expected to be defined in an assembly file.

/**
 * @brief Loads the GDT descriptor into the GDTR register.
 * @param descriptor Pointer to the GDT descriptor.
 */
extern void gdt_load_register(GDTDescriptor *descriptor);

/**
 * @brief Loads the Task State Segment selector into the TR register.
 */
extern void tss_load_register(void);

/**
 * @brief Creates and installs the TSS descriptor in the GDT.
 *
 * This function sets the GDT entry at GDT_TSS_INDEX to point to
 * the global `tss_entry`.
 *
 * This function is only used by `gdt_init` in this file,
 * so it is marked `static`.
 */
static void gdt_install_tss(void) {
    uint32_t base = (uint32_t)&tss_entry;
    uint32_t limit = sizeof(tss_entry) - 1;

    // Access byte: Present, Ring 3 (for user tasks), System, 32-bit TSS
    // P=1, DPL=3, S=0, Type=9 (32-bit TSS)
    uint8_t access = GDT_ACCESS_PRESENT | GDT_ACCESS_RING3 | GDT_TYPE_TSS_32_AVAIL;

    // Flags: 1-byte granularity. 32-bit flag is ignored for TSS.
    uint8_t flags = GDT_FLAG_GRANULARITY_1B;
    GDTEntry tss_descriptor = GDT_ENTRY(base, limit, access, GDT_FLAG_GRANULARITY_1B);
    gdt[GDT_TSS_INDEX] = tss_descriptor;
}

/**
 * @brief Setups the GDT and TSS.
 */
void init_gdt() {
  gdt_load_register(&gdt_discriptor);
  gdt_install_tss();
  tss_load_register();
}
