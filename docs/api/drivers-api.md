# Drivers & Device API

Block device interface:

typedef struct block_device {
    char name[32];
    uint32_t block_size;
    uint64_t num_blocks;
    int (*read)(uint64_t block, uint8_t* buf);
    int (*write)(uint64_t block, const uint8_t* buf);
} block_device_t;

ATA driver:
- int ata_read(uint64_t lba, uint8_t* buf);
- int ata_write(uint64_t lba, const uint8_t* buf);
- int ata_identify(int drive);

Ramdisk:
- void ramdisk_init(void* addr, size_t size);
- int ramdisk_read(uint64_t block, uint8_t* buf);
- int ramdisk_write(uint64_t block, const uint8_t* buf);

TTY / Console:
- void terminal_write(const char* data, size_t size);
- void terminal_writestring(const char* s);

Keyboard:
- void kbd_register_handler(void (*handler)(uint8_t scancode));

Helper APIs for drivers:
- uintptr_t memory_get_phys(void* vaddr);
- void* kmalloc(size_t);
- void kfree(void*);
- void interrupt_save(void);
- void interrupt_restore(void);

Drivers should return standard negative errno values on failure.
