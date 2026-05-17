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

	return ata_read_sector(absolute_sector, buffer);
}

// Change your function signature to return uint32_t
uint32_t init_fat32(void) {
    uint8_t *sector_buffer = (uint8_t *)kmalloc(512);
    uint32_t root_cluster = 0; // Temp variable to hold root cluster target
    
    if (!fat32_read_sector(0, sector_buffer)) {
        printkl("Failed to read FAT32 Boot Record!");
        kfree(sector_buffer);
        return 0;
    }

    // Keep bpb purely local to this initialization function
    struct fat32_bpb *local_bpb = (struct fat32_bpb *)sector_buffer;

    if (local_bpb->boot_signature == 0x29) {
        printkl("Successfully detected FAT32 Volume!");
    }

    // Extract your layout properties safely into your existing globals
    sectors_per_cluster = local_bpb->sectors_per_cluster;
    data_region_start_sector = local_bpb->reserved_sector_count + (local_bpb->num_fats * local_bpb->fat_size_32);
    reserved_sector_count = local_bpb->reserved_sector_count;
    
    // Save the root cluster before we free the underlying memory buffer
    root_cluster = local_bpb->root_cluster;

    kfree(sector_buffer); // Safe to free now!
    return root_cluster;  // Pass it back up to kernel_main
}

uint32_t find_in_directory_cluster(uint32_t cluster_num, const char *target_11_char_name, uint8_t *is_dir) {
    // 1. Calculate the starting physical sector for this cluster
    uint32_t relative_sector = data_region_start_sector + ((cluster_num - 2) * sectors_per_cluster);
    
    // 2. Allocate temporary memory to hold one cluster of data
    uint32_t cluster_size_bytes = sectors_per_cluster * 512;
    uint8_t *cluster_buffer = (uint8_t *)kmalloc(cluster_size_bytes);
    if (!cluster_buffer) return 0;

    // Read all sectors belonging to this cluster
    for (uint32_t i = 0; i < sectors_per_cluster; i++) {
        fat32_read_sector(relative_sector + i, cluster_buffer + (i * 512));
    }

    // 3. Cast the buffer to an array of directory entries
    struct fat32_directory_entry *entries = (struct fat32_directory_entry *)cluster_buffer;
    uint32_t max_entries = cluster_size_bytes / sizeof(struct fat32_directory_entry);

    uint32_t found_cluster = 0;

    for (uint32_t i = 0; i < max_entries; i++) {
        // Stop checking if we hit the end-of-directory marker
        if (entries[i].name[0] == 0x00) {
            break;
        }
        
        // Skip deleted entries or Long File Name (LFN) configuration entries for now
        if (entries[i].name[0] == 0xE5 || entries[i].attr == FAT_ATTR_LONG_NAME) {
            continue;
        }

        // Compare the 11-byte FAT name structure
        // memcmp is safe here because name fields are space-padded and fixed-width
        if (memcmp(entries[i].name, target_11_char_name, 11) == 0) {
            // Reconstruct the 32-bit cluster address from the two 16-bit fields
            found_cluster = ((uint32_t)entries[i].first_cluster_high << 16) | entries[i].first_cluster_low;
            
            // Set the directory tracking flag for the caller
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
    // Each FAT32 entry is 4 bytes (32 bits) long. 
    // 128 entries fit into a single 512-byte sector (512 / 4 = 128).
    uint32_t fat_sector = reserved_sector_count + (current_cluster / 128);
    uint32_t fat_offset = current_cluster % 128;

    uint32_t buffer[128]; // 512-byte temporary sector buffer
    
    // Read the specific FAT sector from disk
    // Note: If you have multiple FAT tables, this reads from the first table (FAT1)
    if (!fat32_read_sector(fat_sector, (uint8_t *)buffer)) {
        return 0x0FFFFFFF; // Return End-of-Chain on hardware read error
    }

    // Ignore the high 4 bits as per the FAT32 specification
    return buffer[fat_offset] & 0x0FFFFFFF;
}

static void format_fat_name(const uint8_t *raw_name, char *dest) {
    int p = 0;
    // Copy filename portion (up to 8 chars), stopping at trailing spaces
    for (int i = 0; i < 8; i++) {
        if (raw_name[i] != ' ') {
            dest[p++] = raw_name[i];
        }
    }
    // Correct check: check if ANY of the extension bytes are not spaces
    if (raw_name[8] != ' ' || raw_name[9] != ' ' || raw_name[10] != ' ') {
        dest[p++] = '.';
        for (int i = 8; i < 11; i++) {
            if (raw_name[i] != ' ') {
                dest[p++] = raw_name[i];
            }
        }
    }
    dest[p] = '\0'; // Explicitly terminate the string safely
}

struct fat32_dir_listing *fat32_list_directory(uint32_t start_cluster) {
    uint32_t cluster_size_bytes = sectors_per_cluster * 512;
    uint32_t entries_per_cluster = cluster_size_bytes / sizeof(struct fat32_directory_entry);
    
    uint32_t total_valid_entries = 0;
    uint32_t current_cluster = start_cluster;
    uint8_t *cluster_buf = (uint8_t *)kmalloc(cluster_size_bytes);
    if (!cluster_buf) return NULL;

    // --- PHASE 1: COUNT VALID ENTRIES ---
    while (current_cluster < 0x0FFFFFF8) {
        uint32_t rel_sector = data_region_start_sector + ((current_cluster - 2) * sectors_per_cluster);
        for (uint32_t i = 0; i < sectors_per_cluster; i++) {
            fat32_read_sector(rel_sector + i, cluster_buf + (i * 512));
        }

        struct fat32_directory_entry *entries = (struct fat32_directory_entry *)cluster_buf;
        for (uint32_t i = 0; i < entries_per_cluster; i++) {
            if (entries[i].name[0] == 0x00) goto counting_done; // End of directory marker
            if (entries[i].name[0] == 0xE5) continue;          // Deleted
            if (entries[i].attr == FAT_ATTR_LONG_NAME) continue; // Long File Name entry

            total_valid_entries++;
        }
        current_cluster = fat32_get_next_cluster(current_cluster);
    }
counting_done:

    // --- PHASE 2: ALLOCATE STRUCTURES ---
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

    // --- PHASE 3: POPULATE STRUCTURES ---
    uint32_t index = 0;
    current_cluster = start_cluster; // Reset to start of chain

    while (current_cluster < 0x0FFFFFF8 && index < total_valid_entries) {
        uint32_t rel_sector = data_region_start_sector + ((current_cluster - 2) * sectors_per_cluster);
        for (uint32_t i = 0; i < sectors_per_cluster; i++) {
            fat32_read_sector(rel_sector + i, cluster_buf + (i * 512));
        }

        struct fat32_directory_entry *entries = (struct fat32_directory_entry *)cluster_buf;
        for (uint32_t i = 0; i < entries_per_cluster; i++) {
            if (entries[i].name[0] == 0x00) goto population_done;
            if (entries[i].name[0] == 0xE5 || entries[i].attr == FAT_ATTR_LONG_NAME) continue;

            // Fill our clean info structure
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

/**
 * Clean up function to properly free a directory listing from the heap
 */
void fat32_free_dir_listing(struct fat32_dir_listing *listing) {
    if (listing) {
        if (listing->entries) {
            kfree(listing->entries);
        }
        kfree(listing);
    }
}

void test_directory_listing(uint32_t root_cluster) {
    struct fat32_dir_listing *list = fat32_list_directory(root_cluster);
    
    if (list == NULL) {
        printkl("Failed to read root directory.");
        return;
    }

    for (uint32_t i = 0; i < list->count; i++) {
        // Output format example: "[DIR] FOLDERNAME" or "[FILE] FILE.TXT (1024 bytes)"
        if (list->entries[i].is_directory) {
            printk("[DIR]  ");
            printkl(list->entries[i].name);
        } else {
            printk("[FILE] ");
            printk(list->entries[i].name);
            // You can implement an integer-to-string print helper later to display size
            printkl(""); 
        }
    }

    // Always free dynamic kernel allocations when you are finished!
    fat32_free_dir_listing(list);
}


/**
 * Reads a complete file from disk into a newly heap-allocated buffer.
 * @param start_cluster The starting cluster of the file from the directory entry.
 * @param file_size The total size of the file in bytes.
 * @return A pointer to the heap-allocated file data, or NULL on failure.
 */
void *fat32_read_file(uint32_t start_cluster, uint32_t file_size) {
    if (file_size == 0 || start_cluster >= 0x0FFFFFF8) {
        return NULL;
    }

    // 1. Allocate a buffer big enough to hold the entire file on the heap
    uint8_t *file_buffer = (uint8_t *)kmalloc(file_size);
    if (!file_buffer) {
        return NULL; 
    }

    uint32_t cluster_size_bytes = sectors_per_cluster * 512;
    
    // Allocate a temporary workspace buffer to read one cluster at a time
    uint8_t *cluster_buf = (uint8_t *)kmalloc(cluster_size_bytes);
    if (!cluster_buf) {
        kfree(file_buffer);
        return NULL;
    }

    uint32_t current_cluster = start_cluster;
    uint32_t bytes_remaining = file_size;
    uint32_t buffer_offset = 0;

    // 2. Loop through the cluster chain until the end-of-chain or all bytes are read
    while (current_cluster < 0x0FFFFFF8 && bytes_remaining > 0) {
        // Calculate the physical hardware sector position for this cluster
        uint32_t rel_sector = data_region_start_sector + ((current_cluster - 2) * sectors_per_cluster);
        
        // Read all hardware sectors belonging to this specific cluster
        for (uint32_t i = 0; i < sectors_per_cluster; i++) {
            fat32_read_sector(rel_sector + i, cluster_buf + (i * 512));
        }

        // Determine how many bytes to copy out of this cluster
        uint32_t bytes_to_copy = (bytes_remaining > cluster_size_bytes) ? cluster_size_bytes : bytes_remaining;
        
        // Copy the data into our master file buffer
        memcpy(file_buffer + buffer_offset, cluster_buf, bytes_to_copy);

        // Update tracking variables
        buffer_offset += bytes_to_copy;
        bytes_remaining -= bytes_to_copy;

        // Advance to the next cluster in the chain via the FAT table
        current_cluster = fat32_get_next_cluster(current_cluster);
    }

    kfree(cluster_buf); // Free temporary workspace
    return file_buffer; // Return the full file data!
}

/**
 * Simple kernel helper to find the first occurrence of a character in a string.
 */
static const char *kernel_strchr(const char *str, int character) {
    while (*str != '\0') {
        if (*str == character) {
            return str;
        }
        str++;
    }
    return NULL;
}

/**
 * Extracts the next token from a path string up to the next '/' or '\0'.
 * Example: Input "/BOOT/SYS/" -> Extracts "BOOT" into output buffer.
 * Returns a pointer to the start of the remaining unparsed path.
 */
static const char *get_next_path_node(const char *path, char *output_buffer) {
    // Skip leading slashes
    while (*path == '/') {
        path++;
    }

    if (*path == '\0') {
        return NULL;
    }

    // Find where this node ends (either at the next slash or end of string)
    const char *next_slash = kernel_strchr(path, '/');
    size_t length = 0;

    if (next_slash != NULL) {
        length = next_slash - path;
    } else {
        // No more slashes, this is the final file/folder name
        length = 0;
        while (path[length] != '\0') {
            length++;
        }
    }

    // Copy characters into our isolated workspace buffer
    for (size_t i = 0; i < length; i++) {
        output_buffer[i] = path[i];
    }
    output_buffer[length] = '\0'; // Cleanly null-terminate it

    return path + length; // Return pointer to the rest of the string
}


/**
 * Resolves a nested virtual file path string and returns its direct metadata.
 * @param path The absolute path string (e.g., "/BOOT/LIMINE.SYS")
 * @param root_cluster The starting root cluster from the BPB configuration.
 * @param out_info A pointer to a fat32_file_info structure to be populated.
 * @return 1 on successful resolution, 0 if any folder or file path node does not exist.
 */
int fat32_resolve_path(const char *path, uint32_t root_cluster, struct fat32_file_info *out_info) {
    if (!path || path[0] == '\0' || !out_info) return 0;

    uint32_t current_cluster = root_cluster;
    char current_node_name[13]; // Safe space for normal 8.3 plus dot and null
    
    const char *path_cursor = path;

    // Loop continuously until we consume the entire path string
    while ((path_cursor = get_next_path_node(path_cursor, current_node_name)) != NULL) {
        
        // 1. Get a clean listing of the directory we are currently standing in
        struct fat32_dir_listing *listing = fat32_list_directory(current_cluster);
        if (!listing) return 0;

        int node_found = 0;

        // 2. Scan every file/folder in this current directory table
        for (uint32_t i = 0; i < listing->count; i++) {
            // Case-insensitive/exact match comparison against our isolated path node name
            if (memcmp(listing->entries[i].name, current_node_name, 13) == 0) {
                
                // Copy the matching structural data out
                for (int j = 0; j < 13; j++) {
                    out_info->name[j] = listing->entries[i].name[j];
                }
                out_info->size = listing->entries[i].size;
                out_info->is_directory = listing->entries[i].is_directory;
                out_info->first_cluster = listing->entries[i].first_cluster;

                node_found = 1;
                break;
            }
        }

        // Clean up memory allocations immediately to prevent kernel memory exhaustion
        fat32_free_dir_listing(listing);

        // If the path item wasn't in this folder, the entire path is invalid
        if (!node_found) {
            return 0; 
        }

        // 3. Setup for the next iteration loop
        if (*path_cursor != '\0') {
            // If there is more path left, the item we just found MUST be a directory
            if (!out_info->is_directory) {
                return 0; // Error: Attempted to treat a file like a directory path node!
            }
            current_cluster = out_info->first_cluster; // Dive deeper down the tree
        }
    }

    return 1; // Success! out_info now holds the metadata of the final file or directory.
}

