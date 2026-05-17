#include <stdint.h>
#include <stddef.h>
#include "headers/vga.h"
#include "headers/gdt.h"
#include "headers/idt.h"
#include "headers/handler.h"
#include "headers/utils.h"
#include "headers/memory.h"
#include "headers/io.h"
#include "headers/fat32.h"
#include "headers/string.h"

void kernel_main(uint32_t magic, uint32_t mbi_addr) {

	terminal_initialize();

	printkl("Loading GDT.");

	gdt_init();
	
	printkl("Loaded GDT, now loading IDT.");

	idt_init();

	init_pic();

	printkl("Loaded IDT and PICs");

	printkl("parsing multiboot2 headers");

	if (magic != 0x36d76289) {
		panic("Invalid multiboot2 magic");
	}

	init_bitmap_allocator(mbi_addr);

	init_paging();			

	// Map virtual 0xC00B8000 directly to the physical VGA text buffer
	//map_page((void*)0xC00B8000, (void*)0xB8000, PAGE_PRESENT | PAGE_WRITE);
	
	// Now writing here safely displays text on screen through the paging system
	//uint16_t *vga_buffer = (uint16_t *)0xC00B8000;
	//vga_buffer[0] = (0x0F << 8) | 'X'; // White text 'X' on black background
	
	//int *array1 = (int *)kmalloc(10 * sizeof(int));
    //array1[0] = 42;
    //array1[9] = 99;

    // Test 2: Triggering dynamic page expansion by asking for a large block
    //void *large_buffer = kmalloc(8000); // Exceeds a single 4KB page


    // Test 3: Freeing memory and ensuring coalescing works
    //kfree(array1);
	//kfree(large_buffer);

//	__asm__ volatile("sti");

//	printkl("Os is now taking input, type something:");	

	// --- FAT32 FILESYSTEM INITIALIZATION & TESTING ---
	/*{
	    // Allocate a temporary 512-byte block on your new heap
	    uint8_t *sector_buffer = (uint8_t *)kmalloc(512);
	    if (!sector_buffer) {
	        printkl("FAT32 Test Error: Out of memory for sector buffer!");
	    } else {
	        // Read the absolute partition start sector (LBA 2048)
	        // by requesting relative sector 0
	        if (!fat32_read_sector(0, sector_buffer)) {
	            printkl("FAT32 Test Error: Failed to read hardware sector!");
	        } else {
	            struct fat32_bpb *bpb = (struct fat32_bpb *)sector_buffer;
	
	            // Save the critical filesystem layout constants globally
	            reserved_sector_count = bpb->reserved_sector_count;
	            sectors_per_cluster = bpb->sectors_per_cluster;
	            
	            // Calculate where the cluster files data region physically starts
	            data_region_start_sector = bpb->reserved_sector_count + 
	                                       (bpb->num_fats * bpb->fat_size_32);
	
	            printkl("FAT32 filesystem successfully initialized!");
	
	            // Grab a full directory list targeting the Root Directory (Cluster 2)
	            struct fat32_dir_listing *list = fat32_list_directory(bpb->root_cluster);
	            
	            if (list == NULL) {
	                printkl("FAT32 Test Error: Could not parse root directory listing.");
	            } else {
	                printkl("--- ROOT DIRECTORY CONTENTS ---");
	                
	                if (list->count == 0) {
	                    printkl("(Directory is completely empty)");
	                }
	
	                for (uint32_t i = 0; i < list->count; i++) {
	                    if (list->entries[i].is_directory) {
	                        printk("[DIR]  ");
	                        printkl(list->entries[i].name);
	                    } else {
	                        printk("[FILE] ");
	                        printkl(list->entries[i].name);
	                    }

	                    if (memcmp(list->entries[i].name, "TEST.TXT", 8) == 0) {
	                            printkl("Found TEST.TXT! Attempting to read content...");
	                            
	                            // 1. Allocate a text buffer that is 1 byte LARGER than the actual file size
	                            char *str_buffer = (char *)kmalloc(list->entries[i].size + 1);
	                            
	                            if (str_buffer) {
	                                // 2. Read the raw file data using your original function
	                                void *raw_file_data = fat32_read_file(list->entries[i].first_cluster, list->entries[i].size);
	                                
	                                if (raw_file_data) {
	                                    // 3. Copy the raw file data into our string buffer
	                                    memcpy(str_buffer, raw_file_data, list->entries[i].size);
	                                    
	                                    // 4. Force place the null terminator at the exact end of the file data
	                                    str_buffer[list->entries[i].size] = '\0';
	                                    
	                                    // 5. Print it safely!
	                                    printk("Content: ");
	                                    printkl(str_buffer);
	                                    
	                                    // Clean up both allocations
	                                    kfree(raw_file_data);
	                                }
	                                kfree(str_buffer);
	                            }
	                        }
	                }
	                
	                printkl("-------------------------------");
	                
	                // Clean up the dynamic list arrays to prevent memory leaks
	                fat32_free_dir_listing(list);
	            }
	        }
	        
	        // Clean up the initial sector buffer allocation
	        kfree(sector_buffer);
	    }
	}
	*/

	

	printkl("=== FAT32 PATH RESOLVER TEST ===");
	
	// Initialize FAT32 and get root cluster
	uint32_t root_cluster = init_fat32();
	
	if (root_cluster == 0) {
	    printkl("FAT32 init failed!");
	    return;
	}
	
	printkl("FAT32 initialized successfully.");
	
	
	// --------------------------------------------------
	// TEST 1: Resolve root directory
	// --------------------------------------------------
	
	struct fat32_file_info info;
	
	if (fat32_resolve_path("/", root_cluster, &info)) {
	    printkl("Resolved root directory successfully.");
	} else {
	    printkl("FAILED: Could not resolve root directory.");
	}
	
	
	// --------------------------------------------------
	// TEST 2: Resolve a DIRECTORY
	// CHANGE THIS TO A REAL DIRECTORY YOU HAVE
	// --------------------------------------------------
	
	if (fat32_resolve_path("/BOOT", root_cluster, &info)) {
	
	    printkl("Resolved directory:");
	
	    printk("Name: ");
	    printkl(info.name);
	
	    printk("Cluster: ");
	    // Replace with your integer print if available
	    printkl("(cluster resolved)");
	
	    if (info.is_directory) {
	        printkl("Confirmed directory.");
	    } else {
	        printkl("ERROR: Expected directory but got file.");
	    }
	
	} else {
	    printkl("FAILED: Could not resolve /BOOT");
	}
	
	
	// --------------------------------------------------
	// TEST 3: Resolve a FILE
	// CHANGE THIS TO A REAL FILE YOU HAVE
	// --------------------------------------------------
	
	if (fat32_resolve_path("/BOOT/TEST.TXT", root_cluster, &info)) {
	
	    printkl("Resolved file:");
	
	    printk("Name: ");
	    printkl(info.name);
	
	    if (!info.is_directory) {
	        printkl("Confirmed regular file.");
	    } else {
	        printkl("ERROR: Expected file but got directory.");
	    }
	
	    // --------------------------------------------------
	    // TEST 4: Read file contents
	    // --------------------------------------------------
	
	    void *file_data = fat32_read_file(info.first_cluster, info.size);
	
	    if (file_data) {
	
	        printkl("Successfully read file contents.");
	
	        // Print first 64 bytes as characters
	        // Useful for text files
	        char *text = (char *)file_data;
	
	        printkl("First bytes:");
	
	        for (uint32_t i = 0; i < 64 && i < info.size; i++) {
	
	            char c = text[i];
	
	            // Replace non-printable chars
	            if (c < 32 || c > 126) {
	                c = '.';
	            }
	
	            terminal_putchar(c);
	        }
	
	        terminal_putchar('\n');
	
	        kfree(file_data);
	
	    } else {
	        printkl("FAILED: Could not read file.");
	    }
	
	} else {
	    printkl("FAILED: Could not resolve /KERNEL.BIN");
	}
	
	
	// --------------------------------------------------
	// TEST 5: Invalid path
	// --------------------------------------------------
	
	if (!fat32_resolve_path("/THIS_DOES_NOT_EXIST.TXT", root_cluster, &info)) {
	    printkl("Correctly rejected invalid path.");
	} else {
	    printkl("ERROR: Invalid path unexpectedly resolved.");
	}
	
	printkl("=== FAT32 TEST COMPLETE ===");
		
	
	// early kernel panic test
	//__asm__ volatile("xor %eax, %eax; div %eax"); 



		

    // Halt
    
}


