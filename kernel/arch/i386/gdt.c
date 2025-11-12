#include <stdint.h>

typedef struct {
  uint16_t LimitLow;    // limit (bits 0-15)
  uint16_t BaseLow;     // base (bits 0-15)
  uint8_t BaseMiddle;   // base (bits 16-23)
  uint8_t Access;       // access
  uint8_t FlagsLimitHi; // limit (bits 16-19) | flags
  uint8_t BaseHigh;     // base (bits 24-31)
} __attribute__((packed)) GDTEntry;

typedef struct {
  uint32_t prevTSS;
  uint32_t ESP0;
  uint32_t SS0;
  uint32_t ESP1;
  uint32_t SS1;
  uint32_t ESP2;
  uint32_t SS2;
  uint32_t CR3;
  uint32_t EIP;
  uint32_t EFLAGS;
  uint32_t EAX;
  uint32_t EDI;
  uint32_t ESI;
  uint32_t EBP;
  uint32_t ESP;
  uint32_t EBX;
  uint32_t ECX;
  uint32_t EDX;
  uint32_t ES;
  uint32_t CS;
  uint32_t SS;
  uint32_t DS;
  uint32_t FS;
  uint32_t GS;
  uint32_t ldt;
  uint16_t trap;
  uint16_t iomap_base;
} __attribute__((packed)) TSSEntry;

typedef struct {
  uint16_t Limit; // sizeof(gdt) - 1
  GDTEntry *Ptr;  // address of GDT
} __attribute__((packed)) GDTDescriptor;

typedef enum {
  GDT_ACCESS_CODE_READABLE = 0x02,
  GDT_ACCESS_DATA_WRITEABLE = 0x02,

  GDT_ACCESS_CODE_CONFORMING = 0x04,
  GDT_ACCESS_DATA_DIRECTION_NORMAL = 0x00,
  GDT_ACCESS_DATA_DIRECTION_DOWN = 0x04,

  GDT_ACCESS_DATA_SEGMENT = 0x10,
  GDT_ACCESS_CODE_SEGMENT = 0x18,

  GDT_ACCESS_DESCRIPTOR_TSS = 0x00,

  GDT_ACCESS_RING0 = 0x00,
  GDT_ACCESS_RING1 = 0x20,
  GDT_ACCESS_RING2 = 0x40,
  GDT_ACCESS_RING3 = 0x60,

  GDT_ACCESS_PRESENT = 0x80,

} GDT_ACCESS;

typedef enum {
  GDT_FLAG_64BIT = 0x20,
  GDT_FLAG_32BIT = 0x40,
  GDT_FLAG_16BIT = 0x00,

  GDT_FLAG_GRANULARITY_1B = 0x00,
  GDT_FLAG_GRANULARITY_4K = 0x80,
} GDT_FLAGS;

// Helper macros
#define GDT_LIMIT_LOW(limit) (limit & 0xFFFF)
#define GDT_BASE_LOW(base) (base & 0xFFFF)
#define GDT_BASE_MIDDLE(base) ((base >> 16) & 0xFF)
#define GDT_FLAGS_LIMIT_HI(limit, flags)                                       \
  (((limit >> 16) & 0xF) | (flags & 0xF0))
#define GDT_BASE_HIGH(base) ((base >> 24) & 0xFF)

#define GDT_ENTRY(base, limit, access, flags)                                  \
  {GDT_LIMIT_LOW(limit),                                                       \
   GDT_BASE_LOW(base),                                                         \
   GDT_BASE_MIDDLE(base),                                                      \
   access,                                                                     \
   GDT_FLAGS_LIMIT_HI(limit, flags),                                           \
   GDT_BASE_HIGH(base)}

static TSSEntry tss_entry = {.SS0 = 0x10,
                    .ESP0 = 0x0,
                    .CS = 0x08 | 0x3,
                    .DS = 0x10 | 0x3,
                    .SS = 0x10 | 0x3,
                    .ES = 0x10 | 0x3,
                    .FS = 0x10 | 0x3,
                    .GS = 0x10 | 0x3};


GDTEntry g_GDT[] = {
    // NULL descriptor
    GDT_ENTRY(0, 0, 0, 0),

    // Kernel 32-bit code segment
    GDT_ENTRY(0, 0xFFFFF,
              GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_CODE_SEGMENT |
                  GDT_ACCESS_CODE_READABLE,
              GDT_FLAG_32BIT | GDT_FLAG_GRANULARITY_4K),

    // Kernel 32-bit data segment
    GDT_ENTRY(0, 0xFFFFF,
              GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_DATA_SEGMENT |
                  GDT_ACCESS_DATA_WRITEABLE,
              GDT_FLAG_32BIT | GDT_FLAG_GRANULARITY_4K),

    // User 32-bit code segment
    GDT_ENTRY(0, 0xFFFFF,
              GDT_ACCESS_PRESENT | GDT_ACCESS_RING3 | GDT_ACCESS_CODE_SEGMENT |
                  GDT_ACCESS_CODE_READABLE,
              GDT_FLAG_32BIT | GDT_FLAG_GRANULARITY_4K),

    // User 32-bit data segment
    GDT_ENTRY(0, 0xFFFFF,
              GDT_ACCESS_PRESENT | GDT_ACCESS_RING3 | GDT_ACCESS_DATA_SEGMENT |
                  GDT_ACCESS_DATA_WRITEABLE,
              GDT_FLAG_32BIT | GDT_FLAG_GRANULARITY_4K),

    // Task State Segment
    {/* Set by `writeTSS` in `GDT_Init` */},
};

GDTDescriptor g_GDTDescriptor = {sizeof(g_GDT) - 1, g_GDT};

extern void GDT_Load(GDTDescriptor *descriptor);
extern void TSS_Load();

void writeTSS() {
  uint32_t base = (uint32_t)&tss_entry;
  uint32_t limit = base + sizeof(TSSEntry);
  GDTEntry TSS = GDT_ENTRY(base, limit, 0xE9, 0x0);
  g_GDT[5] = TSS;
}

void GDT_Init() {
  GDT_Load(&g_GDTDescriptor);
  writeTSS();
  TSS_Load();
}
