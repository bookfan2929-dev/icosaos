#include <stdint.h>
#include <stddef.h>
#include "headers/lvga.h"
#include "headers/gdt.h"
#include "headers/idt.h"
#include "headers/handler.h"
#include "headers/utils.h"
#include "headers/memory.h"
#include "headers/io.h"
#include "headers/fat32.h"
#include "headers/string.h"
#include "headers/vga_graphics.h"
#include "headers/vga_cosmetics.h"
#include "headers/klog.h"
#include "headers/keyboard.h" // Added our new keyboard header

// Multiboot2 tag header structure
struct multiboot_tag {
    uint32_t type;
    uint32_t size;
};

// Multiboot2 framebuffer info structure
struct multiboot_tag_framebuffer {
    uint32_t type; // Will be 8
    uint32_t size;
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t framebuffer_bpp;
    uint8_t framebuffer_type;
    uint8_t reserved;
};

struct acpi_rsdp_10 {
    char signature[8];       // Must be "RSD PTR "
    uint8_t checksum;        // Total sum of all 20 bytes must equal 0
    char oem_id[6];          // OEM identifier string
    uint8_t revision;        // 0 for ACPI 1.0
    uint32_t rsdt_address;   // Physical memory address of the RSDT
} __attribute__((packed));

void kernel_main(uint32_t magic, uint32_t mbi_addr) {

    if (magic != 0x36D76289) {
        return;
    }

    uint32_t mbi_size = *(volatile uint32_t*)mbi_addr;
    uint32_t offset = 8; 

    uint32_t *fb_addr = 0;
    uint32_t fb_width_local = 0;
    uint32_t fb_height_local = 0;
    uint32_t fb_pitch_local = 0;
    uint8_t fb_bpp_local = 0;

    void *acpi_rsdp_phys = NULL;

    while (offset < mbi_size) {
        struct multiboot_tag *tag = (struct multiboot_tag *)(uintptr_t)(mbi_addr + offset);
        
        if (tag->type == 0 && tag->size == 8) {
            break;
        }

        if (tag->type == 8) {
            struct multiboot_tag_framebuffer *fb_tag = (struct multiboot_tag_framebuffer *)tag;

            fb_addr = (uint32_t *)fb_tag->framebuffer_addr;
            fb_width_local = fb_tag->framebuffer_width;
            fb_height_local = fb_tag->framebuffer_height;
            fb_pitch_local = fb_tag->framebuffer_pitch;
            fb_bpp_local = fb_tag->framebuffer_bpp;
            
            vga_init(
                (void *)fb_addr,
                fb_width_local,
                fb_height_local,
                fb_pitch_local,
                fb_bpp_local 
            );
            break;
        } else if (tag->type == 14) {
        	acpi_rsdp_phys = (void *)((uintptr_t)tag + 8);
        }
        
        offset += (tag->size + 7) & ~7;
    }
    
    vga_clear_screen(VGA_BLACK);
    klog_init(VGA_WHITE, VGA_BLACK);
    
    klog_info("Initializing GDT...");
    gdt_init();
    klog_status(LOG_SUCCESS);
    
    klog_info("Initializing IDT...");
    idt_init();
    klog_status(LOG_SUCCESS);

    klog_info("Initializing PIC...");    
    init_pic();
    klog_status(LOG_SUCCESS);

    klog_info("Initializing system allocator...");
    init_bitmap_allocator(mbi_addr);
    klog_status(LOG_SUCCESS);

    klog_info("Initalizing paging...");
    init_paging((uint32_t)fb_addr, fb_pitch_local, fb_height_local);            
    klog_status(LOG_SUCCESS);

    klog_info("Initalizing FAT32 driver...");
    uint32_t root_cluster = init_fat32();
    
    if (root_cluster == 0) {
        klog_status(LOG_FAILED);
        return;
    }
    klog_status(LOG_SUCCESS);
    
    // --------------------------------------------------
    // TEST 1: Resolve root directory
    // --------------------------------------------------
    klog_info("FAT32 root resolution test...");
    struct fat32_file_info info;
    if (fat32_resolve_path("/", root_cluster, &info)) {
        klog_status(LOG_SUCCESS);
    } else {
        klog_status(LOG_FAILED);
        return;
    }
    
    // --------------------------------------------------
    // TEST 2: Resolve a DIRECTORY
    // --------------------------------------------------
    klog_info("FAT32 directory resoultion test...");
    if (fat32_resolve_path("/BOOT", root_cluster, &info)) {
        if (info.is_directory) {
            klog_status(LOG_SUCCESS);
        } else {
            klog_status(LOG_FAILED);
            return;
        }
    } else {
        klog_status(LOG_FAILED);
        return;
    }
    
    // --------------------------------------------------
    // TEST 3: Resolve a FILE
    // --------------------------------------------------
    klog_info("FAT32 file resolution test...");
    if (fat32_resolve_path("/BOOT/SELFTEST.SYS", root_cluster, &info)) {
        klog_status(LOG_SUCCESS);
        klog_info("FAT32 file read test...");
        if (info.is_directory) {
            klog_status(LOG_FAILED);
            return;
        }
        klog_status(LOG_SUCCESS);
        klog_info("FAT32 file contents test...");
    
        // --------------------------------------------------
        // TEST 4: Read file contents
        // --------------------------------------------------
        void *file_data = fat32_read_file(info.first_cluster, info.size);
        if (file_data) {
            char *text = (char *)file_data;

            // Using your updated file-reader setup with null-termination
            if (strcmp("INFO\x0A", text) == 0) {
                klog_status(LOG_SUCCESS);
            } else {
                klog_status(LOG_FAILED);
                kfree(file_data);
                return;
            }
            kfree(file_data);
        } else {
            klog_status(LOG_FAILED);
            return;
        }
    } else {
        klog_status(LOG_FAILED);
        return;
    }

    // --------------------------------------------------
    // TEST 5: Invalid path
    // --------------------------------------------------
    klog_info("FAT32 path resolution rejection test...");
    if (!fat32_resolve_path("/THIS_DOES_NOT_EXIST.TXT", root_cluster, &info)) {
        klog_status(LOG_SUCCESS);
    } else {
        klog_status(LOG_FAILED);
        return;
    }
    
    // --------------------------------------------------
    // INTERACTIVE SHELL PIPELINE START
    // --------------------------------------------------
// --------------------------------------------------
    // INTERACTIVE SHELL PIPELINE START
    // --------------------------------------------------
    vga_print("\n--- All tests passed! Entering OS Shell ---\n\n", 0x00FF00FF, VGA_BLACK);

    // 1. Clear out any junk inside the keyboard hardware controller buffer
    inb(0x60); 

    // 2. Open the CPU interrupt gates so IRQ1/Interrupt 33 can start executing
    __asm__ volatile("sti");

	__asm__ volatile("int $0x40");
	/*
    char cmd_buffer[64];

    // Stateful Navigation: Default our working path directory to the root cluster
    uint32_t current_dir_cluster = root_cluster;

    while (1) {
        // Print shell prompt
        vga_print("my_kernel> ", 0x00FFFF00, VGA_BLACK);
        
        // This blocks internally, tracking backspaces and echoes to VGA
        kernel_read_line(cmd_buffer, 64); 
        
        // Command Execution parsing
        if (strcmp("help", cmd_buffer) == 0) {
            vga_print("Available commands: help, clear, ls, cd, cat\n", VGA_WHITE, VGA_BLACK);
        } 
        else if (strcmp("clear", cmd_buffer) == 0) {
            vga_clear_screen(VGA_BLACK);
            vga_set_cursor(0, 0); // Reset text rendering positions to the top left
        } 
        // --- 1. LS Command (Handles current directory & arguments) ---
        else if (strcmp("ls", cmd_buffer) == 0 || strncmp("ls ", cmd_buffer, 3) == 0) {
            uint32_t target_cluster = current_dir_cluster;
            int path_valid = 1;

            // If a path argument was supplied (e.g. "ls /BOOT" or "ls BOOT")
            if (strncmp("ls ", cmd_buffer, 3) == 0) {
                char *path_arg = cmd_buffer + 3;
                struct fat32_file_info ls_target_info;

                // Resolve path using our current context directory as base
                if (fat32_resolve_path(path_arg, current_dir_cluster, &ls_target_info)) {
                    if (ls_target_info.is_directory) {
                        target_cluster = ls_target_info.first_cluster;
                    } else {
                        vga_print("ls: Error: Target is a file, not a directory.\n", 0xFF0000FF, VGA_BLACK);
                        path_valid = 0;
                    }
                } else {
                    vga_print("ls: Error: Directory not found.\n", 0xFF0000FF, VGA_BLACK);
                    path_valid = 0;
                }
            }

            if (path_valid) {
                // Fetch valid entries using your fat32 directory listing function
                struct fat32_dir_listing *list = fat32_list_directory(target_cluster);
                if (list != NULL) {
                    for (uint32_t i = 0; i < list->count; i++) {
                        if (list->entries[i].is_directory) {
                            vga_print("[DIR]  ", 0x0000FFFF, VGA_BLACK); // Render directories in Blue
                            vga_print(list->entries[i].name, VGA_WHITE, VGA_BLACK);
                        } else {
                            vga_print("[FILE] ", 0x00FF00FF, VGA_BLACK); // Render files in Green
                            vga_print(list->entries[i].name, VGA_WHITE, VGA_BLACK);
                        }
                        vga_print("\n", VGA_WHITE, VGA_BLACK);
                    }
                    // Clean up dynamic heap allocations using your native tracking method
                    fat32_free_dir_listing(list);
                } else {
                    vga_print("ls: Error reading target directory directory map.\n", 0xFF0000FF, VGA_BLACK);
                }
            }
        }
        // --- 2. CD Command (Stateful traversal) ---
        else if (strncmp("cd ", cmd_buffer, 3) == 0) {
            char *path_arg = cmd_buffer + 3;

            // Special case shortcut: "cd /" resets to root directory
            if (strcmp("/", path_arg) == 0) {
                current_dir_cluster = root_cluster;
            } else {
                struct fat32_file_info cd_target_info;

                if (fat32_resolve_path(path_arg, current_dir_cluster, &cd_target_info)) {
                    if (cd_target_info.is_directory) {
                        current_dir_cluster = cd_target_info.first_cluster;
                    } else {
                        vga_print("cd: Error: Target is a file, not a directory.\n", 0xFF0000FF, VGA_BLACK);
                    }
                } else {
                    vga_print("cd: Error: No such directory exists.\n", 0xFF0000FF, VGA_BLACK);
                }
            }
        }
        // --- 3. CAT Command (Aware of context directory cluster) ---
        else if (strncmp("cat ", cmd_buffer, 4) == 0) {
            char* filename = cmd_buffer + 4; 
            
            struct fat32_file_info cat_info;
            // Updated to use current_dir_cluster instead of hardcoded root_cluster
            if (fat32_resolve_path(filename, current_dir_cluster, &cat_info)) {
                if (!cat_info.is_directory) {
                    void *cat_data = fat32_read_file(cat_info.first_cluster, cat_info.size);
                    if (cat_data) {
                        vga_print((char *)cat_data, VGA_WHITE, VGA_BLACK);
                        vga_print("\n", VGA_WHITE, VGA_BLACK);
                        kfree(cat_data);
                    } else {
                        vga_print("Error: Could not read file content.\n", 0xFF0000FF, VGA_BLACK);
                    }
                } else {
                    vga_print("Error: Target path is a directory, not a file.\n", 0xFF0000FF, VGA_BLACK);
                }
            } else {
                vga_print("Error: File not found.\n", 0xFF0000FF, VGA_BLACK);
            }
        } 
        else if (cmd_buffer[0] != '\0') {
            vga_print("Unknown Command. Type 'help' for options.\n", 0xFF0000FF, VGA_BLACK);
        }
    }
	*/	

    // Safety fallback halt loop if loop ever unbends
    while(1) {
        __asm__ volatile("hlt");
    }
}
