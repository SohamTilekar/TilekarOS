#include "fat.h"
#include "buffer.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "kmalloc.h"

// --- Internal FAT Helpers ---

static bool fat_is_eoc(const fat_filesystem_t* fs, uint32_t value) {
    if (fs->type == FAT_TYPE_FAT16) return value >= 0xFFF8;
    return value >= 0xFF8;
}

static uint16_t fat_eoc_value(const fat_filesystem_t* fs) {
    return (fs->type == FAT_TYPE_FAT16) ? 0xFFFF : 0x0FFF;
}

static uint16_t fat_get_fat_entry(fat_filesystem_t* fs, uint32_t cluster) {
    if (fs->type == FAT_TYPE_FAT16) {
        uint32_t fat_offset = cluster * 2;
        uint32_t fat_sector_lba = fs->fat_start + (fat_offset / fs->bpb.bytes_per_sector);
        uint32_t ent_offset = fat_offset % fs->bpb.bytes_per_sector;
        buffer_t* buf = buffer_get(fs->dev, fat_sector_lba);
        if (!buf) return 0xFFFF;
        uint16_t res = *(uint16_t*)&buf->data[ent_offset];
        buffer_release(buf);
        return res;
    }

    uint32_t fat_offset = cluster + (cluster / 2);
    uint32_t fat_sector_lba = fs->fat_start + (fat_offset / fs->bpb.bytes_per_sector);
    uint32_t ent_offset = fat_offset % fs->bpb.bytes_per_sector;
    buffer_t* buf = buffer_get(fs->dev, fat_sector_lba);
    if (!buf) return 0xFFF;
    uint16_t table_value;
    if (ent_offset == fs->bpb.bytes_per_sector - 1) {
        table_value = buf->data[ent_offset];
        buffer_release(buf);
        buf = buffer_get(fs->dev, fat_sector_lba + 1);
        if (!buf) return 0xFFF;
        table_value |= ((uint16_t)buf->data[0]) << 8;
    } else {
        table_value = *(uint16_t*)&buf->data[ent_offset];
    }
    uint16_t res = (cluster & 0x0001) ? (table_value >> 4) : (table_value & 0x0FFF);
    buffer_release(buf);
    return res;
}

static void fat_set_fat_entry(fat_filesystem_t* fs, uint32_t cluster, uint16_t value) {
    if (fs->type == FAT_TYPE_FAT16) {
        uint32_t fat_offset = cluster * 2;
        uint32_t fat_sector_lba = fs->fat_start + (fat_offset / fs->bpb.bytes_per_sector);
        uint32_t ent_offset = fat_offset % fs->bpb.bytes_per_sector;
        for (int i = 0; i < fs->bpb.num_fats; i++) {
            uint32_t current_fat_sector = fat_sector_lba + (i * fs->bpb.fat_size_16);
            buffer_t* buf = buffer_get(fs->dev, current_fat_sector);
            if (!buf) continue;
            *(uint16_t*)&buf->data[ent_offset] = value;
            buf->dirty = 1;
            buffer_release(buf);
        }
        return;
    }

    uint32_t fat_offset = cluster + (cluster / 2);
    uint32_t fat_sector_lba = fs->fat_start + (fat_offset / fs->bpb.bytes_per_sector);
    uint32_t ent_offset = fat_offset % fs->bpb.bytes_per_sector;
    for (int i = 0; i < fs->bpb.num_fats; i++) {
        uint32_t current_fat_sector = fat_sector_lba + (i * fs->bpb.fat_size_16);
        buffer_t* buf = buffer_get(fs->dev, current_fat_sector);
        if (!buf) continue;
        if (cluster & 0x0001) {
            value <<= 4;
            if (ent_offset == fs->bpb.bytes_per_sector - 1) {
                buf->data[ent_offset] = (buf->data[ent_offset] & 0x0F) | (value & 0xF0);
                buf->dirty = 1; buffer_release(buf);
                buf = buffer_get(fs->dev, current_fat_sector + 1);
                if (!buf) continue;
                buf->data[0] = (uint8_t)(value >> 8);
            } else {
                *(uint16_t*)&buf->data[ent_offset] = (*(uint16_t*)&buf->data[ent_offset] & 0x000F) | value;
            }
        } else {
            value &= 0x0FFF;
            if (ent_offset == fs->bpb.bytes_per_sector - 1) {
                buf->data[ent_offset] = (uint8_t)value;
                buf->dirty = 1; buffer_release(buf);
                buf = buffer_get(fs->dev, current_fat_sector + 1);
                if (!buf) continue;
                buf->data[0] = (buf->data[0] & 0xF0) | (uint8_t)(value >> 8);
            } else {
                *(uint16_t*)&buf->data[ent_offset] = (*(uint16_t*)&buf->data[ent_offset] & 0xF000) | value;
            }
        }
        buf->dirty = 1; buffer_release(buf);
    }
}

static uint32_t fat_find_free_cluster(fat_filesystem_t* fs) {
    uint32_t total_sectors = (fs->bpb.total_sectors_16 == 0) ? fs->bpb.total_sectors_32 : fs->bpb.total_sectors_16;
    uint32_t data_sectors = total_sectors - fs->data_start;
    uint32_t cluster_count = data_sectors / fs->bpb.sectors_per_cluster;
    for (uint32_t i = 2; i < cluster_count + 2; i++) {
        if (fat_get_fat_entry(fs, i) == 0) return i;
    }
    return 0;
}

static void to_fat_name(const char* name, uint8_t* out_name, uint8_t* out_ext) {
    memset(out_name, ' ', 8);
    memset(out_ext, ' ', 3);
    const char* dot = strchr(name, '.');
    int name_len = dot ? (int)(dot - name) : (int)strlen(name);
    if (name_len > 8) name_len = 8;
    memcpy(out_name, name, name_len);
    if (dot) {
        int ext_len = (int)strlen(dot + 1);
        if (ext_len > 3) ext_len = 3;
        memcpy(out_ext, dot + 1, ext_len);
    }
    for (int i = 0; i < 8; i++) if (out_name[i] >= 'a' && out_name[i] <= 'z') out_name[i] -= 32;
    for (int i = 0; i < 3; i++) if (out_ext[i] >= 'a' && out_ext[i] <= 'z') out_ext[i] -= 32;
}

// --- API Implementation ---

int fat_init(fat_filesystem_t* fs, device_t* dev) {
    fs->dev = dev;
    buffer_t* buf = buffer_get(dev, 0);
    if (!buf) return -1;
    memcpy(&fs->bpb, buf->data, sizeof(fat_boot_record_t));
    buffer_release(buf);
    if (fs->bpb.boot_signature != 0x28 && fs->bpb.boot_signature != 0x29) return -2;
    fs->fat_start = fs->bpb.reserved_sector_count;
    fs->root_dir_start = fs->fat_start + (fs->bpb.num_fats * fs->bpb.fat_size_16);
    fs->data_start = fs->root_dir_start + (fs->bpb.root_entry_count * sizeof(fat_directory_entry_t) + 511) / 512;
    uint32_t total_sectors = (fs->bpb.total_sectors_16 == 0) ? fs->bpb.total_sectors_32 : fs->bpb.total_sectors_16;
    uint32_t data_sectors = total_sectors - fs->data_start;
    uint32_t cluster_count = data_sectors / fs->bpb.sectors_per_cluster;
    if (cluster_count < 4085) fs->type = FAT_TYPE_FAT12;
    else if (cluster_count < 65525) fs->type = FAT_TYPE_FAT16;
    else fs->type = FAT_TYPE_FAT32;
    return 0;
}

static bool find_in_directory(fat_filesystem_t* fs, uint32_t cluster, const char* component, fat_directory_entry_t* out_entry, uint32_t* out_sector, uint32_t* out_idx) {
    uint8_t fat_name[8], fat_ext[3];
    to_fat_name(component, fat_name, fat_ext);
    bool is_root = (cluster == 0);
    uint32_t current_cluster = cluster;
    do {
        uint32_t start_sector, num_sectors;
        if (is_root) {
            start_sector = fs->root_dir_start;
            num_sectors = (fs->bpb.root_entry_count * sizeof(fat_directory_entry_t) + 511) / 512;
        } else {
            start_sector = fs->data_start + (current_cluster - 2) * fs->bpb.sectors_per_cluster;
            num_sectors = fs->bpb.sectors_per_cluster;
        }
        for (uint32_t s = 0; s < num_sectors; s++) {
            buffer_t* buf = buffer_get(fs->dev, start_sector + s);
            if (!buf) break;
            fat_directory_entry_t* entries = (fat_directory_entry_t*)buf->data;
            for (uint32_t i = 0; i < 512 / sizeof(fat_directory_entry_t); i++) {
                if (entries[i].filename[0] == 0) { buffer_release(buf); return false; }
                if (entries[i].filename[0] == 0xE5) continue;
                if (entries[i].attributes == FAT_ATTR_LFN) continue;
                if (memcmp(entries[i].filename, fat_name, 8) == 0 && memcmp(entries[i].extension, fat_ext, 3) == 0) {
                    if (out_entry) *out_entry = entries[i];
                    if (out_sector) *out_sector = start_sector + s;
                    if (out_idx) *out_idx = i;
                    buffer_release(buf); return true;
                }
            }
            buffer_release(buf);
        }
        if (is_root) break;
        current_cluster = fat_get_fat_entry(fs, current_cluster);
    } while (!fat_is_eoc(fs, current_cluster));
    return false;
}

static uint32_t resolve_path(fat_filesystem_t* fs, const char* path, fat_directory_entry_t* last_entry) {
    uint32_t current_cluster = 0;
    const char* p = path;
    if (*p == '/') p++;
    while (*p) {
        char component[256];
        const char* end = strchr(p, '/');
        if (end) {
            size_t len = end - p;
            if (len >= 256) len = 255;
            strncpy(component, p, len);
            component[len] = '\0';
            p = end + 1;
        } else {
            strcpy(component, p);
            p += strlen(p);
        }
        if (component[0] == '\0') continue;
        fat_directory_entry_t entry;
        if (!find_in_directory(fs, current_cluster, component, &entry, NULL, NULL)) return 0xFFFFFFFF;
        if (last_entry) *last_entry = entry;
        current_cluster = entry.first_cluster_low;
    }
    return current_cluster;
}

void fat_list_dir(fat_filesystem_t* fs, const char* path) {
    uint32_t cluster = 0;
    if (strcmp(path, "/") != 0 && strcmp(path, "") != 0) {
        cluster = resolve_path(fs, path, NULL);
        if (cluster == 0xFFFFFFFF) { printf("Directory not found: %s\n", path); return; }
    }
    printf("Listing directory %s:\n", path);
    bool is_root = (cluster == 0);
    uint32_t current_cluster = cluster;
    do {
        uint32_t start_sector, num_sectors;
        if (is_root) {
            start_sector = fs->root_dir_start;
            num_sectors = (fs->bpb.root_entry_count * sizeof(fat_directory_entry_t) + 511) / 512;
        } else {
            start_sector = fs->data_start + (current_cluster - 2) * fs->bpb.sectors_per_cluster;
            num_sectors = fs->bpb.sectors_per_cluster;
        }
        for (uint32_t s = 0; s < num_sectors; s++) {
            buffer_t* buf = buffer_get(fs->dev, start_sector + s);
            if (!buf) break;
            fat_directory_entry_t* entries = (fat_directory_entry_t*)buf->data;
            for (uint32_t i = 0; i < 512 / sizeof(fat_directory_entry_t); i++) {
                if (entries[i].filename[0] == 0) { buffer_release(buf); return; }
                if (entries[i].filename[0] == 0xE5) continue;
                if (entries[i].attributes == FAT_ATTR_LFN) continue;
                char name[9], ext[4];
                memcpy(name, entries[i].filename, 8); name[8] = 0;
                memcpy(ext, entries[i].extension, 3); ext[3] = 0;
                for (int k = 7; k >= 0 && name[k] == ' '; k--) name[k] = 0;
                for (int k = 2; k >= 0 && ext[k] == ' '; k--) ext[k] = 0;
                printf("  %s%s%s  %u bytes (attr: %x)\n", name, ext[0] ? "." : "", ext, entries[i].file_size, entries[i].attributes);
            }
            buffer_release(buf);
        }
        if (is_root) break;
        current_cluster = fat_get_fat_entry(fs, current_cluster);
    } while (!fat_is_eoc(fs, current_cluster));
}

void fat_list_root_dir(fat_filesystem_t* fs) { fat_list_dir(fs, "/"); }

static int create_entry(fat_filesystem_t* fs, uint32_t dir_cluster, const char* name, uint8_t attr, uint32_t first_cluster, uint32_t size) {
    uint32_t entry_sector = 0, entry_idx = 0;
    bool found = false;
    bool is_root = (dir_cluster == 0);
    uint32_t current_cluster = dir_cluster;
    do {
        uint32_t start_sector, num_sectors;
        if (is_root) {
            start_sector = fs->root_dir_start;
            num_sectors = (fs->bpb.root_entry_count * sizeof(fat_directory_entry_t) + 511) / 512;
        } else {
            start_sector = fs->data_start + (current_cluster - 2) * fs->bpb.sectors_per_cluster;
            num_sectors = fs->bpb.sectors_per_cluster;
        }
        for (uint32_t s = 0; s < num_sectors; s++) {
            buffer_t* buf = buffer_get(fs->dev, start_sector + s);
            if (!buf) break;
            fat_directory_entry_t* entries = (fat_directory_entry_t*)buf->data;
            for (uint32_t i = 0; i < 512 / sizeof(fat_directory_entry_t); i++) {
                if (entries[i].filename[0] == 0 || entries[i].filename[0] == 0xE5) {
                    entry_sector = start_sector + s;
                    entry_idx = i;
                    found = true;
                    buffer_release(buf);
                    break;
                }
            }
            if (found) break;
            buffer_release(buf);
        }
        if (found || is_root) break;
        uint32_t next = fat_get_fat_entry(fs, current_cluster);
        if (fat_is_eoc(fs, next)) return -1;
        current_cluster = next;
    } while (!fat_is_eoc(fs, current_cluster));
    if (!found) return -1;
    buffer_t* buf = buffer_get(fs->dev, entry_sector);
    fat_directory_entry_t* entry = &((fat_directory_entry_t*)buf->data)[entry_idx];
    memset(entry, 0, sizeof(fat_directory_entry_t));
    to_fat_name(name, entry->filename, entry->extension);
    entry->attributes = attr;
    entry->first_cluster_low = (uint16_t)first_cluster;
    entry->file_size = size;
    buf->dirty = 1;
    buffer_release(buf);
    return 0;
}

int fat_mkdir(fat_filesystem_t* fs, const char* path) {
    char parent_path[256], name[256];
    strcpy(parent_path, path);
    const char* last_slash = strrchr(parent_path, '/');
    if (!last_slash) return -1;
    strcpy(name, last_slash + 1);
    if (last_slash == parent_path) strcpy(parent_path, "/");
    else { char* ls_mut = (char*)last_slash; *ls_mut = 0; }
    uint32_t parent_cluster = 0;
    if (strcmp(parent_path, "/") != 0) {
        parent_cluster = resolve_path(fs, parent_path, NULL);
        if (parent_cluster == 0xFFFFFFFF) return -1;
    }
    uint32_t new_cluster = fat_find_free_cluster(fs);
    if (!new_cluster) return -2;
    fat_set_fat_entry(fs, new_cluster, fat_eoc_value(fs));
    uint32_t cluster_start = fs->data_start + (new_cluster - 2) * fs->bpb.sectors_per_cluster;
    buffer_t* buf = buffer_get(fs->dev, cluster_start);
    if (!buf) return -3;
    memset(buf->data, 0, fs->bpb.bytes_per_sector);
    for (uint32_t s = 1; s < fs->bpb.sectors_per_cluster; s++) {
        buffer_t* extra = buffer_get(fs->dev, cluster_start + s);
        if (!extra) continue;
        memset(extra->data, 0, fs->bpb.bytes_per_sector);
        extra->dirty = 1;
        buffer_release(extra);
    }
    fat_directory_entry_t* entries = (fat_directory_entry_t*)buf->data;
    memset(entries[0].filename, ' ', 8); entries[0].filename[0] = '.';
    memset(entries[0].extension, ' ', 3);
    entries[0].attributes = FAT_ATTR_DIRECTORY;
    entries[0].first_cluster_low = (uint16_t)new_cluster;
    memset(entries[1].filename, ' ', 8); entries[1].filename[0] = '.'; entries[1].filename[1] = '.';
    memset(entries[1].extension, ' ', 3);
    entries[1].attributes = FAT_ATTR_DIRECTORY;
    entries[1].first_cluster_low = (uint16_t)parent_cluster;
    buf->dirty = 1; buffer_release(buf);
    int res = create_entry(fs, parent_cluster, name, FAT_ATTR_DIRECTORY, new_cluster, 0);
    buffer_flush_all(fs->dev);
    return res;
}

int fat_create_file(fat_filesystem_t* fs, const char* path, const uint8_t* data, uint32_t size) {
    char parent_path[256], name[256];
    strcpy(parent_path, path);
    const char* last_slash = strrchr(parent_path, '/');
    if (!last_slash) return -1;
    strcpy(name, last_slash + 1);
    if (last_slash == parent_path) strcpy(parent_path, "/");
    else { char* ls_mut = (char*)last_slash; *ls_mut = 0; }
    uint32_t parent_cluster = 0;
    if (strcmp(parent_path, "/") != 0) {
        parent_cluster = resolve_path(fs, parent_path, NULL);
        if (parent_cluster == 0xFFFFFFFF) return -1;
    }
    uint32_t first_cluster = 0;
    uint32_t original_size = size;
    if (size > 0) {
        uint32_t cluster_size = fs->bpb.bytes_per_sector * fs->bpb.sectors_per_cluster;
        uint32_t needed = (size + cluster_size - 1) / cluster_size;
        uint32_t prev_cluster = 0;
        for (uint32_t i = 0; i < needed; i++) {
            uint32_t clus = fat_find_free_cluster(fs);
            if (!clus) return -2;
            fat_set_fat_entry(fs, clus, fat_eoc_value(fs));
            if (i == 0) first_cluster = clus;
            else fat_set_fat_entry(fs, prev_cluster, clus);
            uint32_t cluster_start = fs->data_start + (clus - 2) * fs->bpb.sectors_per_cluster;
            uint32_t bytes_in_cluster = (size > cluster_size) ? cluster_size : size;
            uint32_t remaining_in_cluster = bytes_in_cluster;
            uint32_t data_offset = i * cluster_size;
            for (uint32_t s = 0; s < fs->bpb.sectors_per_cluster; s++) {
                buffer_t* buf = buffer_get(fs->dev, cluster_start + s);
                if (!buf) continue;
                uint32_t to_copy = (remaining_in_cluster > fs->bpb.bytes_per_sector) ? fs->bpb.bytes_per_sector : remaining_in_cluster;
                memset(buf->data, 0, fs->bpb.bytes_per_sector);
                if (to_copy > 0) {
                    memcpy(buf->data, data + data_offset + (s * fs->bpb.bytes_per_sector), to_copy);
                    remaining_in_cluster -= to_copy;
                }
                buf->dirty = 1;
                buffer_release(buf);
            }
            size -= bytes_in_cluster;
            prev_cluster = clus;
        }
    }
    int res = create_entry(fs, parent_cluster, name, FAT_ATTR_ARCHIVE, first_cluster, original_size);
    buffer_flush_all(fs->dev);
    return res;
}

int fat_read_file(fat_filesystem_t* fs, const char* path, uint8_t* buffer, uint32_t max_size) {
    fat_directory_entry_t entry;
    if (resolve_path(fs, path, &entry) == 0xFFFFFFFF) return -1;
    if (entry.attributes & FAT_ATTR_DIRECTORY) return -2;
    uint32_t current_cluster = entry.first_cluster_low;
    uint32_t total_read = 0;
    while (current_cluster >= 2 && !fat_is_eoc(fs, current_cluster) && total_read < max_size) {
        uint32_t cluster_start_sector = fs->data_start + (current_cluster - 2) * fs->bpb.sectors_per_cluster;
        for (uint32_t s = 0; s < fs->bpb.sectors_per_cluster && total_read < max_size; s++) {
            buffer_t* buf = buffer_get(fs->dev, cluster_start_sector + s);
            if (!buf) return total_read;
            uint32_t to_copy = (entry.file_size - total_read);
            if (to_copy > fs->bpb.bytes_per_sector) to_copy = fs->bpb.bytes_per_sector;
            if (total_read + to_copy > max_size) to_copy = max_size - total_read;
            memcpy(buffer + total_read, buf->data, to_copy);
            buffer_release(buf);
            total_read += to_copy;
            if (total_read >= entry.file_size) break;
        }
        if (total_read >= entry.file_size) break;
        current_cluster = fat_get_fat_entry(fs, current_cluster);
    }
    return total_read;
}

// --- VFS Integration ---

typedef struct {
    fat_filesystem_t* fs;
    fat_directory_entry_t entry;
    uint32_t parent_cluster;
} fat_node_data_t;

static int fat_vfs_read(file_t* file, void* buffer, uint32_t size) {
    fat_node_data_t* data = (fat_node_data_t*)file->node->fs_data;
    uint32_t current_cluster = data->entry.first_cluster_low;
    uint32_t total_read = 0;
    uint32_t offset = file->position;
    if (offset >= data->entry.file_size) return 0;
    if (offset + size > data->entry.file_size) size = data->entry.file_size - offset;
    
    uint32_t cluster_size = data->fs->bpb.bytes_per_sector * data->fs->bpb.sectors_per_cluster;
    uint32_t skip_clusters = offset / cluster_size;
    for (uint32_t i = 0; i < skip_clusters; i++) {
        current_cluster = fat_get_fat_entry(data->fs, current_cluster);
        if (fat_is_eoc(data->fs, current_cluster)) return 0;
    }
    
    uint32_t offset_in_cluster = offset % cluster_size;
    uint32_t sector_in_cluster = offset_in_cluster / data->fs->bpb.bytes_per_sector;
    uint32_t offset_in_sector = offset_in_cluster % data->fs->bpb.bytes_per_sector;

    while (total_read < size && current_cluster >= 2 && !fat_is_eoc(data->fs, current_cluster)) {
        uint32_t cluster_start_sector = data->fs->data_start + (current_cluster - 2) * data->fs->bpb.sectors_per_cluster;
        
        for (uint32_t s = sector_in_cluster; s < data->fs->bpb.sectors_per_cluster && total_read < size; s++) {
            buffer_t* buf = buffer_get(data->fs->dev, cluster_start_sector + s);
            if (!buf) return total_read;
            uint32_t to_copy = data->fs->bpb.bytes_per_sector - offset_in_sector;
            if (to_copy > (size - total_read)) to_copy = size - total_read;
            
            memcpy((uint8_t*)buffer + total_read, buf->data + offset_in_sector, to_copy);
            buffer_release(buf);
            
            total_read += to_copy;
            offset_in_sector = 0; // Only first sector read might have offset
        }
        
        sector_in_cluster = 0; // Only first cluster read might start at non-zero sector
        current_cluster = fat_get_fat_entry(data->fs, current_cluster);
    }
    file->position += total_read;
    return total_read;
}

static vnode_t* fat_vfs_lookup(vnode_t* parent, const char* name) {
    fat_node_data_t* parent_data = (fat_node_data_t*)parent->fs_data;
    uint32_t cluster = parent_data->entry.first_cluster_low;
    if (parent->type == VFS_TYPE_DIRECTORY && parent_data->parent_cluster == 0xFFFFFFFF) cluster = 0;
    fat_directory_entry_t entry;
    uint32_t sector, idx;
    if (!find_in_directory(parent_data->fs, cluster, name, &entry, &sector, &idx)) return NULL;
    vnode_t* node = kcalloc(1, sizeof(vnode_t));
    node->type = (entry.attributes & FAT_ATTR_DIRECTORY) ? VFS_TYPE_DIRECTORY : VFS_TYPE_FILE;
    node->size = entry.file_size;
    node->ops = parent->ops; node->dev = parent->dev;
    fat_node_data_t* node_data = kcalloc(1, sizeof(fat_node_data_t));
    node_data->fs = parent_data->fs; node_data->entry = entry; node_data->parent_cluster = cluster;
    node->fs_data = node_data;
    return node;
}

static int fat_vfs_mkdir(vnode_t* parent, const char* name) {
    fat_node_data_t* parent_data = (fat_node_data_t*)parent->fs_data;
    uint32_t parent_cluster = parent_data->entry.first_cluster_low;
    if (parent_data->parent_cluster == 0xFFFFFFFF) parent_cluster = 0;
    
    uint32_t new_cluster = fat_find_free_cluster(parent_data->fs);
    if (!new_cluster) return -2;
    fat_set_fat_entry(parent_data->fs, new_cluster, fat_eoc_value(parent_data->fs));
    uint32_t cluster_start = parent_data->fs->data_start + (new_cluster - 2) * parent_data->fs->bpb.sectors_per_cluster;
    buffer_t* buf = buffer_get(parent_data->fs->dev, cluster_start);
    if (!buf) return -3;
    memset(buf->data, 0, parent_data->fs->bpb.bytes_per_sector);
    for (uint32_t s = 1; s < parent_data->fs->bpb.sectors_per_cluster; s++) {
        buffer_t* extra = buffer_get(parent_data->fs->dev, cluster_start + s);
        if (!extra) continue;
        memset(extra->data, 0, parent_data->fs->bpb.bytes_per_sector);
        extra->dirty = 1;
        buffer_release(extra);
    }
    fat_directory_entry_t* entries = (fat_directory_entry_t*)buf->data;
    memset(entries[0].filename, ' ', 8); entries[0].filename[0] = '.';
    memset(entries[0].extension, ' ', 3); entries[0].attributes = FAT_ATTR_DIRECTORY;
    entries[0].first_cluster_low = (uint16_t)new_cluster;
    memset(entries[1].filename, ' ', 8); entries[1].filename[0] = '.'; entries[1].filename[1] = '.';
    memset(entries[1].extension, ' ', 3); entries[1].attributes = FAT_ATTR_DIRECTORY;
    entries[1].first_cluster_low = (uint16_t)parent_cluster;
    buf->dirty = 1; buffer_release(buf);
    int res = create_entry(parent_data->fs, parent_cluster, name, FAT_ATTR_DIRECTORY, new_cluster, 0);
    buffer_flush_all(parent_data->fs->dev);
    return res;
}

static int fat_vfs_unlink(vnode_t* parent, const char* name) {
    fat_node_data_t* parent_data = (fat_node_data_t*)parent->fs_data;
    uint32_t parent_cluster = parent_data->entry.first_cluster_low;
    if (parent_data->parent_cluster == 0xFFFFFFFF) parent_cluster = 0;
    fat_directory_entry_t entry;
    uint32_t sector_lba, idx;
    if (!find_in_directory(parent_data->fs, parent_cluster, name, &entry, &sector_lba, &idx)) return -1;
    if (entry.attributes & FAT_ATTR_DIRECTORY) return -2;
    // Mark as deleted
    buffer_t* buf = buffer_get(parent_data->fs->dev, sector_lba);
    ((fat_directory_entry_t*)buf->data)[idx].filename[0] = 0xE5;
    buf->dirty = 1; buffer_release(buf);
    // Free cluster chain
    uint32_t curr = entry.first_cluster_low;
    while (curr >= 2 && !fat_is_eoc(parent_data->fs, curr)) {
        uint32_t next = fat_get_fat_entry(parent_data->fs, curr);
        fat_set_fat_entry(parent_data->fs, curr, 0);
        curr = next;
    }
    buffer_flush_all(parent_data->fs->dev);
    return 0;
}

static int fat_vfs_readdir(vnode_t* dir, uint32_t index, vfs_dirent_t* out) {
    fat_node_data_t* data = (fat_node_data_t*)dir->fs_data;
    uint32_t cluster = data->entry.first_cluster_low;
    if (data->parent_cluster == 0xFFFFFFFF) cluster = 0;
    bool is_root = (cluster == 0);
    uint32_t current_cluster = cluster;
    uint32_t count = 0;
    do {
        uint32_t start_sector, num_sectors;
        if (is_root) {
            start_sector = data->fs->root_dir_start;
            num_sectors = (data->fs->bpb.root_entry_count * sizeof(fat_directory_entry_t) + 511) / 512;
        } else {
            start_sector = data->fs->data_start + (current_cluster - 2) * data->fs->bpb.sectors_per_cluster;
            num_sectors = data->fs->bpb.sectors_per_cluster;
        }
        for (uint32_t s = 0; s < num_sectors; s++) {
            buffer_t* buf = buffer_get(data->fs->dev, start_sector + s);
            if (!buf) return -1;
            fat_directory_entry_t* entries = (fat_directory_entry_t*)buf->data;
            for (uint32_t i = 0; i < 512 / sizeof(fat_directory_entry_t); i++) {
                if (entries[i].filename[0] == 0) { buffer_release(buf); return -1; }
                if (entries[i].filename[0] == 0xE5 || entries[i].attributes == FAT_ATTR_LFN) continue;
                if (count == index) {
                    char name[9], ext[4];
                    memcpy(name, entries[i].filename, 8); name[8] = 0;
                    memcpy(ext, entries[i].extension, 3); ext[3] = 0;
                    for (int k = 7; k >= 0 && name[k] == ' '; k--) name[k] = 0;
                    for (int k = 2; k >= 0 && ext[k] == ' '; k--) ext[k] = 0;
                    if (ext[0]) sprintf(out->name, "%s.%s", name, ext); else strcpy(out->name, name);
                    out->size = entries[i].file_size;
                    out->type = (entries[i].attributes & FAT_ATTR_DIRECTORY) ? VFS_TYPE_DIRECTORY : VFS_TYPE_FILE;
                    buffer_release(buf); return 0;
                }
                count++;
            }
            buffer_release(buf);
        }
        if (is_root) break;
        current_cluster = fat_get_fat_entry(data->fs, current_cluster);
    } while (!fat_is_eoc(data->fs, current_cluster));
    return -1;
}

static vfs_ops_t fat_vfs_ops = {
    .read = fat_vfs_read,
    .write = NULL,
    .lookup = fat_vfs_lookup,
    .mkdir = fat_vfs_mkdir,
    .rmdir = NULL, // Similar to unlink
    .unlink = fat_vfs_unlink,
    .readdir = fat_vfs_readdir
};

vnode_t* fat_mount(device_t* dev) {
    fat_filesystem_t* fs = kmalloc(sizeof(fat_filesystem_t));
    if (fat_init(fs, dev) != 0) { kfree(fs); return NULL; }
    vnode_t* root = kmalloc(sizeof(vnode_t));
    root->type = VFS_TYPE_DIRECTORY; root->size = 0; root->ops = &fat_vfs_ops; root->dev = dev;
    fat_node_data_t* data = kmalloc(sizeof(fat_node_data_t));
    data->fs = fs; memset(&data->entry, 0, sizeof(fat_directory_entry_t));
    data->entry.attributes = FAT_ATTR_DIRECTORY; data->parent_cluster = 0xFFFFFFFF;
    root->fs_data = data;
    return root;
}

void fat_format(device_t* dev, const char* label) {
    fat_boot_record_t bpb;
    memset(&bpb, 0, sizeof(bpb));
    bpb.boot_jmp[0] = 0xEB; bpb.boot_jmp[1] = 0x3C; bpb.boot_jmp[2] = 0x90;
    memcpy(bpb.oem_name, "TILEKAR ", 8);
    bpb.bytes_per_sector = 512; bpb.reserved_sector_count = 1; bpb.num_fats = 2;
    bool use_fat16 = dev->total_sectors > 8400; // FAT12 practical limit around 4K clusters with 1 sector/cluster
    bpb.sectors_per_cluster = 1;
    bpb.root_entry_count = use_fat16 ? 512 : 224;
    bpb.total_sectors_16 = dev->total_sectors;
    bpb.media_type = 0xF8; bpb.sectors_per_track = 18; bpb.num_heads = 2;
    bpb.drive_number = 0x00; bpb.boot_signature = 0x29; bpb.volume_id = 0x12345678;
    memset(bpb.volume_label, ' ', 11); memcpy(bpb.volume_label, label, strlen(label) > 11 ? 11 : strlen(label));
    if (use_fat16) {
        uint32_t root_dir_sectors = (bpb.root_entry_count * sizeof(fat_directory_entry_t) + (bpb.bytes_per_sector - 1)) / bpb.bytes_per_sector;
        uint32_t fat_size = 1;
        while (1) {
            uint32_t data_sectors = bpb.total_sectors_16 - (bpb.reserved_sector_count + (bpb.num_fats * fat_size) + root_dir_sectors);
            uint32_t cluster_count = data_sectors / bpb.sectors_per_cluster;
            uint32_t needed_fat_sectors = ((cluster_count + 2) * 2 + (bpb.bytes_per_sector - 1)) / bpb.bytes_per_sector;
            if (needed_fat_sectors == fat_size) break;
            fat_size = needed_fat_sectors;
        }
        bpb.fat_size_16 = fat_size;
        memcpy(bpb.file_system_type, "FAT16   ", 8);
    } else {
        bpb.fat_size_16 = 9;
        memcpy(bpb.file_system_type, "FAT12   ", 8);
    }
    buffer_t* buf = buffer_get(dev, 0);
    memset(buf->data, 0, 512); memcpy(buf->data, &bpb, sizeof(bpb));
    buf->data[510] = 0x55; buf->data[511] = 0xAA;
    buf->dirty = 1; buffer_release(buf);
    for (int f = 0; f < bpb.num_fats; f++) {
        uint32_t fat_start = bpb.reserved_sector_count + (f * bpb.fat_size_16);
        buf = buffer_get(dev, fat_start);
        memset(buf->data, 0, 512);
        buf->data[0] = bpb.media_type;
        buf->data[1] = 0xFF;
        buf->data[2] = 0xFF;
        if (use_fat16) buf->data[3] = 0xFF;
        buf->dirty = 1; buffer_release(buf);
        for (int i = 1; i < bpb.fat_size_16; i++) {
            buf = buffer_get(dev, fat_start + i);
            memset(buf->data, 0, 512); buf->dirty = 1; buffer_release(buf);
        }
    }
    uint32_t root_dir_start = bpb.reserved_sector_count + (bpb.num_fats * bpb.fat_size_16);
    uint32_t root_dir_sectors = (bpb.root_entry_count * sizeof(fat_directory_entry_t) + 511) / 512;
    for (uint32_t i = 0; i < root_dir_sectors; i++) {
        buf = buffer_get(dev, root_dir_start + i);
        memset(buf->data, 0, 512); buf->dirty = 1; buffer_release(buf);
    }
    buffer_flush_all(dev);
    printf("Disk %s formatted as FAT12 with label: %s\n", dev->name, label);
}
