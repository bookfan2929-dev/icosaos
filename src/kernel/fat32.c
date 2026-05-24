#include "headers/fat32.h"
#include "headers/ata.h"
#include "headers/memory.h"
#include "headers/utils.h"
#include "headers/string.h"

#define PARTITION_START_SECTOR 2048

uint32_t data_region_start_sector = 0;
uint32_t sectors_per_cluster = 0;
uint32_t reserved_sector_count = 0;

struct fat32_bpb *bpb;

int fat32_read_sector(uint32_t relative_sector, uint8_t *buffer) {
    uint32_t absolute_sector = PARTITION_START_SECTOR + relative_sector;
    
    // SAFETY CHECK: The ATA controller requires physical memory addresses.
    // If your kernel heap allocator uses high virtual mappings (e.g., above 4MB),
    // this buffer must be translated to a physical address or handled via an identity-mapped bounce buffer.
    return ata_read_sector(absolute_sector, buffer);
}

// Change your function signature to return uint32_t
uint32_t init_fat32(void) {
    // Allocation must occur in identity-mapped memory spaces
    uint8_t *sector_buffer = (uint8_t *)kmalloc(512);
    if (!sector_buffer) return 0;
    
    uint32_t root_cluster = 0; 
    
    if (!fat32_read_sector(0, sector_buffer)) {
        lprintkl("Failed to read FAT32 Boot Record!");
        kfree(sector_buffer);
        return 0;
    }

    struct fat32_bpb *local_bpb = (struct fat32_bpb *)sector_buffer;

    if (local_bpb->boot_signature == 0x29) {
        lprintkl("Successfully detected FAT32 Volume!");
    }

    sectors_per_cluster = local_bpb->sectors_per_cluster;
    data_region_start_sector = local_bpb->reserved_sector_count + (local_bpb->num_fats * local_bpb->fat_size_32);
    reserved_sector_count = local_bpb->reserved_sector_count;
    
    root_cluster = local_bpb->root_cluster;

    kfree(sector_buffer); 
    return root_cluster;  
}

uint32_t find_in_directory_cluster(uint32_t cluster_num, const char *target_11_char_name, uint8_t *is_dir) {
    uint32_t relative_sector = data_region_start_sector + ((cluster_num - 2) * sectors_per_cluster);
    
    uint32_t cluster_size_bytes = sectors_per_cluster * 512;
    
    // kmalloc here must return a buffer within the 4MB identity mapped zone
    uint8_t *cluster_buffer = (uint8_t *)kmalloc(cluster_size_bytes);
    if (!cluster_buffer) return 0;

    for (uint32_t i = 0; i < sectors_per_cluster; i++) {
        fat32_read_sector(relative_sector + i, cluster_buffer + (i * 512));
    }

    struct fat32_directory_entry *entries = (struct fat32_directory_entry *)cluster_buffer;
    uint32_t max_entries = cluster_size_bytes / sizeof(struct fat32_directory_entry);

    uint32_t found_cluster = 0;

    for (uint32_t i = 0; i < max_entries; i++) {
        if (entries[i].name[0] == 0x00) {
            break;
        }
        
        if (entries[i].name[0] == 0xE5 || entries[i].attr == FAT_ATTR_LONG_NAME) {
            continue;
        }

        if (memcmp(entries[i].name, target_11_char_name, 11) == 0) {
            found_cluster = ((uint32_t)entries[i].first_cluster_high << 16) | entries[i].first_cluster_low;
            
            if (is_dir) {
                *is_dir = (entries[i].attr & FAT_ATTR_DIRECTORY) ? 1 : 0;
            }
            break;
        }
    }

    kfree(cluster_buffer);
    return found_cluster;
}

uint32_t fat32_get_next_cluster(uint32_t current_cluster) {
    uint32_t fat_sector = reserved_sector_count + (current_cluster / 128);
    uint32_t fat_offset = current_cluster % 128;

    // FIX: Allocating on the stack might cross non-identity paged boundaries 
    // depending on where the stack pointer (ESP) is relocated.
    // Force a safe heap allocation inside the identity map.
    uint32_t *buffer = (uint32_t *)kmalloc(512);
    if (!buffer) return 0x0FFFFFFF;
    
    if (!fat32_read_sector(fat_sector, (uint8_t *)buffer)) {
        kfree(buffer);
        return 0x0FFFFFFF; 
    }

    uint32_t next_cluster = buffer[fat_offset] & 0x0FFFFFFF;
    kfree(buffer);
    
    return next_cluster;
}

static void format_fat_name(const uint8_t *raw_name, char *dest) {
    int p = 0;
    for (int i = 0; i < 8; i++) {
        if (raw_name[i] != ' ') {
            dest[p++] = raw_name[i];
        }
    }
    if (raw_name[8] != ' ' || raw_name[9] != ' ' || raw_name[10] != ' ') {
        dest[p++] = '.';
        for (int i = 8; i < 11; i++) {
            if (raw_name[i] != ' ') {
                dest[p++] = raw_name[i];
            }
        }
    }
    dest[p] = '\0'; 
}

struct fat32_dir_listing *fat32_list_directory(uint32_t start_cluster) {
    uint32_t cluster_size_bytes = sectors_per_cluster * 512;
    uint32_t entries_per_cluster = cluster_size_bytes / sizeof(struct fat32_directory_entry);
    
    uint32_t total_valid_entries = 0;
    uint32_t current_cluster = start_cluster;
    uint8_t *cluster_buf = (uint8_t *)kmalloc(cluster_size_bytes);
    if (!cluster_buf) return NULL;

    while (current_cluster < 0x0FFFFFF8) {
        uint32_t rel_sector = data_region_start_sector + ((current_cluster - 2) * sectors_per_cluster);
        for (uint32_t i = 0; i < sectors_per_cluster; i++) {
            fat32_read_sector(rel_sector + i, cluster_buf + (i * 512));
        }

        struct fat32_directory_entry *entries = (struct fat32_directory_entry *)cluster_buf;
        for (uint32_t i = 0; i < entries_per_cluster; i++) {
            if (entries[i].name[0] == 0x00) goto counting_done; 
            if (entries[i].name[0] == 0xE5) continue;          
            if (entries[i].attr == FAT_ATTR_LONG_NAME) continue; 

            total_valid_entries++;
        }
        current_cluster = fat32_get_next_cluster(current_cluster);
    }
counting_done:

    struct fat32_dir_listing *listing = (struct fat32_dir_listing *)kmalloc(sizeof(struct fat32_dir_listing));
    if (!listing) {
        kfree(cluster_buf);
        return NULL;
    }
    listing->count = total_valid_entries;
    listing->entries = (struct fat32_file_info *)kmalloc(total_valid_entries * sizeof(struct fat32_file_info));
    if (!listing->entries) {
        kfree(listing);
        kfree(cluster_buf);
        return NULL;
    }

    uint32_t index = 0;
    current_cluster = start_cluster; 

    while (current_cluster < 0x0FFFFFF8 && index < total_valid_entries) {
        uint32_t rel_sector = data_region_start_sector + ((current_cluster - 2) * sectors_per_cluster);
        for (uint32_t i = 0; i < sectors_per_cluster; i++) {
            fat32_read_sector(rel_sector + i, cluster_buf + (i * 512));
        }

        struct fat32_directory_entry *entries = (struct fat32_directory_entry *)cluster_buf;
        for (uint32_t i = 0; i < entries_per_cluster; i++) {
            if (entries[i].name[0] == 0x00) goto population_done;
            if (entries[i].name[0] == 0xE5 || entries[i].attr == FAT_ATTR_LONG_NAME) continue;

            format_fat_name(entries[i].name, listing->entries[index].name);
            listing->entries[index].size = entries[i].file_size;
            listing->entries[index].is_directory = (entries[i].attr & FAT_ATTR_DIRECTORY) ? 1 : 0;
            listing->entries[index].first_cluster = ((uint32_t)entries[i].first_cluster_high << 16) | entries[i].first_cluster_low;

            index++;
        }
        current_cluster = fat32_get_next_cluster(current_cluster);
    }
population_done:

    kfree(cluster_buf);
    return listing;
}

void fat32_free_dir_listing(struct fat32_dir_listing *listing) {
    if (listing) {
        if (listing->entries) {
            kfree(listing->entries);
        }
        kfree(listing);
    }
}

void *fat32_read_file(uint32_t start_cluster, uint32_t file_size) {
    if (file_size == 0 || start_cluster >= 0x0FFFFFF8) {
        return NULL;
    }

    uint8_t *file_buffer = (uint8_t *)kmalloc(file_size + 1);
    if (!file_buffer) {
        return NULL; 
    }

    uint32_t cluster_size_bytes = sectors_per_cluster * 512;
    uint8_t *cluster_buf = (uint8_t *)kmalloc(cluster_size_bytes);
    if (!cluster_buf) {
        kfree(file_buffer);
        return NULL;
    }

    uint32_t current_cluster = start_cluster;
    uint32_t bytes_remaining = file_size;
    uint32_t buffer_offset = 0;

    while (current_cluster < 0x0FFFFFF8 && bytes_remaining > 0) {
        uint32_t rel_sector = data_region_start_sector + ((current_cluster - 2) * sectors_per_cluster);
        
        for (uint32_t i = 0; i < sectors_per_cluster; i++) {
            fat32_read_sector(rel_sector + i, cluster_buf + (i * 512));
        }

        uint32_t bytes_to_copy = (bytes_remaining > cluster_size_bytes) ? cluster_size_bytes : bytes_remaining;
        
        memcpy(file_buffer + buffer_offset, cluster_buf, bytes_to_copy);

        buffer_offset += bytes_to_copy;
        bytes_remaining -= bytes_to_copy;

        current_cluster = fat32_get_next_cluster(current_cluster);
    }

    kfree(cluster_buf);
    file_buffer[file_size] = '\0';
    return file_buffer; 
}

static const char *get_next_path_node(const char *path, char *output_buffer) {
    while (*path == '/') {
        path++;
    }
    if (*path == '\0') {
        return NULL;
    }
    size_t length = 0;
    while (path[length] != '/' && path[length] != '\0') {
        output_buffer[length] = path[length];
        length++;
    }
    output_buffer[length] = '\0';
    return path + length;
}

static void fat32_normalize_path_token(char *str) {
    while (*str) {
        if (*str >= 'a' && *str <= 'z') {
            *str = *str - ('a' - 'A');
        }
        str++;
    }
}

int fat32_resolve_path(const char *path, uint32_t root_cluster, struct fat32_file_info *out_info) {
    if (!path || !out_info) {
        return 0;
    }

    if (path[0] == '/' && path[1] == '\0') {
        out_info->name[0] = '/';
        out_info->name[1] = '\0';
        out_info->size = 0;
        out_info->is_directory = 1;
        out_info->first_cluster = root_cluster;
        return 1;
    }

    uint32_t current_cluster = root_cluster;
    char current_node_name[13];
    const char *path_cursor = path;

    while ((path_cursor = get_next_path_node(path_cursor, current_node_name)) != NULL) {
        fat32_normalize_path_token(current_node_name);

        struct fat32_dir_listing *listing = fat32_list_directory(current_cluster);
        if (!listing) {
            return 0;
        }

        int node_found = 0;
        for (uint32_t i = 0; i < listing->count; i++) {
            if (strcmp(listing->entries[i].name, current_node_name) == 0) {
                *out_info = listing->entries[i];
                node_found = 1;
                break;
            }
        }

        fat32_free_dir_listing(listing);

        if (!node_found) {
            return 0;
        }

        while (*path_cursor == '/') {
            path_cursor++;
        }

        if (*path_cursor != '\0') {
            if (!out_info->is_directory) {
                return 0;
            }
            current_cluster = out_info->first_cluster;
        }
    }
    return 1;
}
