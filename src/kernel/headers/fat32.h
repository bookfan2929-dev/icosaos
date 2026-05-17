#ifndef FAT32_H
#define FAT32_H
#include <stdint.h>
#include "ata.h"

extern uint32_t reserved_sector_count;
extern uint32_t data_region_start_sector;
extern uint32_t sectors_per_cluster;


#pragma pack(push, 1) // Force the compiler to pack variables tight with no padding
struct fat32_bpb {
    uint8_t  bootjmp[3];
    uint8_t  oem_name[8];
    uint16_t bytes_per_sector;     // Crucial: Usually 512
    uint8_t  sectors_per_cluster;  // Crucial: Math base for parsing clusters
    uint16_t reserved_sector_count;// Number of sectors before the first FAT table
    uint8_t  num_fats;             // Crucial: Usually 2
    uint16_t root_entry_count;     // Always 0 for FAT32
    uint16_t total_sectors_16;     // Always 0 for FAT32
    uint8_t  media_type;
    uint16_t fat_size_16;          // Always 0 for FAT32
    uint16_t sectors_per_track;
    uint16_t num_heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;     // Total size of the partition in sectors
    
    // --- FAT32 Extended Fields ---
    uint32_t fat_size_32;          // Size of a single FAT table in sectors
    uint16_t ext_flags;
    uint16_t fs_version;
    uint32_t root_cluster;         // Crucial: The starting cluster of the root directory
    uint16_t fs_info_sector;
    uint16_t backup_boot_sector;
    uint8_t  reserved[12];
    uint8_t  drive_number;
    uint8_t  reserved1;
    uint8_t  boot_signature;
    uint32_t volume_id;
    uint8_t  volume_label[11];
    uint8_t  fs_type[8];           // Will literally say "FAT32   "
};
#pragma pack(pop)

extern struct fat32_bpb *bpb;

#pragma pack(push, 1)
struct fat32_directory_entry {
    uint8_t  name[11];      // 0-7: Name, 8-10: Extension
    uint8_t  attr;         // Attributes (0x10 = Subdirectory, 0x20 = Archive/File)
    uint8_t  nt_res;
    uint8_t  crt_time_ten;
    uint16_t crt_time;
    uint16_t crt_date;
    uint16_t lst_acc_date;
    uint16_t first_cluster_high; // Top 16 bits of this file's starting cluster
    uint16_t wrt_time;
    uint16_t wrt_date;
    uint16_t first_cluster_low;  // Bottom 16 bits of this file's starting cluster
    uint32_t file_size;          // File size in bytes (0 for subdirectories)
};
#pragma pack(pop)

// Attribute flags
#define FAT_ATTR_READ_ONLY 0x01
#define FAT_ATTR_HIDDEN    0x02
#define FAT_ATTR_SYSTEM    0x04
#define FAT_ATTR_VOLUME_ID 0x08
#define FAT_ATTR_DIRECTORY 0x10
#define FAT_ATTR_ARCHIVE   0x20
#define FAT_ATTR_LONG_NAME 0x0F // LFN tracking (skip these for raw 8.3 parsing)

struct fat32_file_info {
    char name[13];         // <-- CHANGE THIS from 'char name;'
    uint32_t size;         
    uint8_t is_directory;  
    uint32_t first_cluster;
};

// The wrapper structure returned by your directory listing function
struct fat32_dir_listing {
    uint32_t count;                 // Number of files/folders found
    struct fat32_file_info *entries;// Pointer to the heap-allocated array
};

int fat32_read_sector(uint32_t relative_sector, uint8_t *buffer);
uint32_t init_fat32(void);
uint32_t find_in_directory_cluster(uint32_t cluster_num, const char *target_11_char_name, uint8_t *is_dir);
uint32_t fat32_get_next_cluster(uint32_t current_cluster);
struct fat32_dir_listing *fat32_list_directory(uint32_t start_cluster);
void fat32_free_dir_listing(struct fat32_dir_listing *listing);
void test_directory_listing(uint32_t root_cluster);
void *fat32_read_file(uint32_t start_cluser, uint32_t file_size);
int fat32_resolve_path(const char *path, uint32_t root_cluster, struct fat32_file_info *out_info);

#endif
