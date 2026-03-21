#ifndef FAT_H
#define FAT_H

#include <stdint.h>
#include <stdbool.h>
#include "devices.h"
#include "vfs.h"

#pragma pack(push, 1)

typedef struct {
    uint8_t boot_jmp[3];
    uint8_t oem_name[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sector_count;
    uint8_t num_fats;
    uint16_t root_entry_count;
    uint16_t total_sectors_16;
    uint8_t media_type;
    uint16_t fat_size_16;
    uint16_t sectors_per_track;
    uint16_t num_heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;

    // Extended Boot Record
    uint8_t drive_number;
    uint8_t reserved;
    uint8_t boot_signature;
    uint32_t volume_id;
    uint8_t volume_label[11];
    uint8_t file_system_type[8];
} fat_boot_record_t;

typedef struct {
    uint8_t filename[8];
    uint8_t extension[3];
    uint8_t attributes;
    uint8_t reserved;
    uint8_t creation_time_ms;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_access_date;
    uint16_t first_cluster_high; // FAT32 only
    uint16_t last_write_time;
    uint16_t last_write_date;
    uint16_t first_cluster_low;
    uint32_t file_size;
} fat_directory_entry_t;

#pragma pack(pop)

#define FAT_ATTR_READ_ONLY 0x01
#define FAT_ATTR_HIDDEN    0x02
#define FAT_ATTR_SYSTEM    0x04
#define FAT_ATTR_VOLUME_ID 0x08
#define FAT_ATTR_DIRECTORY 0x10
#define FAT_ATTR_ARCHIVE   0x20
#define FAT_ATTR_LFN       0x0F

typedef enum {
    FAT_TYPE_FAT12,
    FAT_TYPE_FAT16,
    FAT_TYPE_FAT32
} fat_type_t;

typedef struct {
    device_t* dev;
    fat_boot_record_t bpb;
    uint32_t root_dir_start;
    uint32_t data_start;
    uint32_t fat_start;
    fat_type_t type;
} fat_filesystem_t;

// VFS integration
vnode_t* fat_mount(device_t* dev);

// Existing API
int fat_init(fat_filesystem_t* fs, device_t* dev);
void fat_list_root_dir(fat_filesystem_t* fs);
void fat_list_dir(fat_filesystem_t* fs, const char* path);
void fat_format(device_t* dev, const char* label);
int fat_mkdir(fat_filesystem_t* fs, const char* path);
int fat_create_file(fat_filesystem_t* fs, const char* path, const uint8_t* data, uint32_t size);
int fat_read_file(fat_filesystem_t* fs, const char* path, uint8_t* buffer, uint32_t max_size);

#endif // FAT_H
