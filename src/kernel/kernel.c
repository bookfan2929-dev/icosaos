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
    // Color info fields follow here, but are omitted for simplicity
};



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

    while (offset < mbi_size) {
        struct multiboot_tag *tag = (struct multiboot_tag *)(uintptr_t)(mbi_addr + offset);
        
        if (tag->type == 0 && tag->size == 8) {
            break;
        }

        // Inside your multiboot parsing loop block inside kernel_main:
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
                fb_bpp_local // <-- Critical new parameter argument
            );
            break;
        }
        

        // Advance using explicit variable math to avoid standard pointer optimization pitfalls
        offset += (tag->size + 7) & ~7;
    }
    
	//vga_init(fb_addr, fb_width_local, fb_height_local, fb_pitch_local);

	vga_clear_screen(0x0000000F);

	vga_print("Loaded kernel\n",0x00FFFFFF, 0);

	//terminal_initialize();

	//lprintkl("Loading GDT.");

	gdt_init();
	
	vga_print("initialized gdt\n",0x00FFFFFF,0);
	
	//lprintkl("Loaded GDT, now loading IDT.");

	idt_init();

	vga_print("initialized idt\n",0x00FFFFFF,0);

	init_pic();

	vga_print("initialized pic\n",0x00FFFFFF,0);

	//lprintkl("Loaded IDT and PICs");

	//lprintkl("parsing multiboot2 headers");



	init_bitmap_allocator(mbi_addr);

	vga_print("initalized bitmap allocator\n", 0x00FFFFFF, 0);

	init_paging((uint32_t)fb_addr, fb_pitch_local, fb_height_local);			

	vga_print("initialized allocator and paging\n",0x00FFFFFF,0);

	inb(0x60); // Read and discard whatever is currently in the buffer
	__asm__ volatile("sti"); // Now safely open the interrupt floodgates!
	

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

//	lprintkl("Os is now taking input, type something:");	
	/*
	
	lprintkl("=== FAT32 PATH RESOLVER TEST ===");
	
	// Initialize FAT32 and get root cluster
	uint32_t root_cluster = init_fat32();
	
	if (root_cluster == 0) {
	    lprintkl("FAT32 init failed!");
	    return;
	}
	
	lprintkl("FAT32 initialized successfully.");
	
	
	// --------------------------------------------------
	// TEST 1: Resolve root directory
	// --------------------------------------------------
	
	struct fat32_file_info info;
	
	if (fat32_resolve_path("/", root_cluster, &info)) {
	    lprintkl("Resolved root directory successfully.");
	} else {
	    lprintkl("FAILED: Could not resolve root directory.");
	}
	
	
	// --------------------------------------------------
	// TEST 2: Resolve a DIRECTORY
	// CHANGE THIS TO A REAL DIRECTORY YOU HAVE
	// --------------------------------------------------
	
	if (fat32_resolve_path("/BOOT", root_cluster, &info)) {
	
	    lprintkl("Resolved directory:");
	
	    lprintk("Name: ");
	    lprintkl(info.name);
	
	    lprintk("Cluster: ");
	    // Replace with your integer print if available
	    lprintkl("(cluster resolved)");
	
	    if (info.is_directory) {
	        lprintkl("Confirmed directory.");
	    } else {
	        lprintkl("ERROR: Expected directory but got file.");
	    }
	
	} else {
	    lprintkl("FAILED: Could not resolve /BOOT");
	}
	
	
	// --------------------------------------------------
	// TEST 3: Resolve a FILE
	// CHANGE THIS TO A REAL FILE YOU HAVE
	// --------------------------------------------------
	
	if (fat32_resolve_path("/BOOT/TEST.TXT", root_cluster, &info)) {
	
	    lprintkl("Resolved file:");
	
	    lprintk("Name: ");
	    lprintkl(info.name);
	
	    if (!info.is_directory) {
	        lprintkl("Confirmed regular file.");
	    } else {
	        lprintkl("ERROR: Expected file but got directory.");
	    }
	
	    // --------------------------------------------------
	    // TEST 4: Read file contents
	    // --------------------------------------------------
	
	    void *file_data = fat32_read_file(info.first_cluster, info.size);
	
	    if (file_data) {
	
	        lprintkl("Successfully read file contents.");
	
	        // Print first 64 bytes as characters
	        // Useful for text files
	        char *text = (char *)file_data;
	
	        lprintkl("First bytes:");
	
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
	        lprintkl("FAILED: Could not read file.");
	    }
	
	} else {
	    lprintkl("FAILED: Could not resolve /KERNEL.BIN");
	}
	
	
	// --------------------------------------------------
	// TEST 5: Invalid path
	// --------------------------------------------------
	
	if (!fat32_resolve_path("/THIS_DOES_NOT_EXIST.TXT", root_cluster, &info)) {
	    lprintkl("Correctly rejected invalid path.");
	} else {
	    lprintkl("ERROR: Invalid path unexpectedly resolved.");
	}
	
	lprintkl("=== FAT32 TEST COMPLETE ===");
	*/	
	// early kernel panic test
	//__asm__ volatile("xor %eax, %eax; div %eax"); 


	


		

    // Halt
	
    return;
}


