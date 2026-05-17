#include <stdint.h>
#include <stddef.h>

struct multiboot_tag {
	uint32_t type;
	uint32_t size;
};

struct multiboot_mmap_entry {
	uint64_t addr;
    uint64_t len;
    uint32_t type;
    uint32_t reserved;	
};

#define PAGE_SIZE 4096

extern uint8_t end; // End of kernel image from linker script
uint8_t *bitmap = NULL;
uint32_t total_pages = 0;
uint32_t bitmap_size_bytes = 0;

void init_bitmap_allocator(uint32_t mbi_addr) {
    uint64_t max_address = 0;
    
    // --- PHASE 1: FIND MAX MEMORY ---
    struct multiboot_tag *tag = (struct multiboot_tag *)(mbi_addr + 8);
    while (tag->type != 0) {
        if (tag->type == 6) {
            uint32_t entry_size = *(uint32_t *)((uint8_t *)tag + 8);
            struct multiboot_mmap_entry *entry = (struct multiboot_mmap_entry *)((uint8_t *)tag + 16);
            uint32_t num_entries = (tag->size - 16) / entry_size;

            for (uint32_t i = 0; i < num_entries; i++) {
                if (entry->type == 1) {
                    uint64_t end_addr = entry->addr + entry->len;
                    if (end_addr > max_address) {
                        max_address = end_addr;
                    }
                }
                entry = (struct multiboot_mmap_entry *)((uint8_t *)entry + entry_size);
            }
        }
        tag = (struct multiboot_tag *)((uint8_t *)tag + ((tag->size + 7) & ~7));
    }

    // --- PHASE 2: SETUP BITMAP METADATA ---
    total_pages = max_address / PAGE_SIZE;
    bitmap_size_bytes = total_pages / 8;
    
    // Place bitmap right after the kernel
    bitmap = &end; 
    
    // Mark ALL memory as reserved (1) initially
    for (uint32_t i = 0; i < bitmap_size_bytes; i++) {
        bitmap[i] = 0xFF; 
    }

    // --- PHASE 3: FREE AVAILABLE RAM ---
    tag = (struct multiboot_tag *)(mbi_addr + 8); // Reset tag pointer
    while (tag->type != 0) {
        if (tag->type == 6) {
            uint32_t entry_size = *(uint32_t *)((uint8_t *)tag + 8);
            struct multiboot_mmap_entry *entry = (struct multiboot_mmap_entry *)((uint8_t *)tag + 16);
            uint32_t num_entries = (tag->size - 16) / entry_size;

            for (uint32_t i = 0; i < num_entries; i++) {
                if (entry->type == 1) {
                    // Loop through this available chunk page by page
                    for (uint64_t addr = entry->addr; addr < entry->addr + entry->len; addr += PAGE_SIZE) {
                        uint32_t page_index = addr / PAGE_SIZE;
                        
                        // Clear the bit to mark it FREE (0)
                        bitmap[page_index / 8] &= ~(1 << (page_index % 8));
                    }
                }
                entry = (struct multiboot_mmap_entry *)((uint8_t *)entry + entry_size);
            }
        }
        tag = (struct multiboot_tag *)((uint8_t *)tag + ((tag->size + 7) & ~7));
    }

    // --- PHASE 4: PROTECT KERNEL & BITMAP ---
    uint32_t kernel_start_page = 0x100000 / PAGE_SIZE; // Standard GRUB load address (1MB)
    uint32_t bitmap_end_address = (uint32_t)bitmap + bitmap_size_bytes;
    uint32_t protect_end_page = (bitmap_end_address + PAGE_SIZE - 1) / PAGE_SIZE;

    for (uint32_t page = kernel_start_page; page < protect_end_page; page++) {
        // Re-reserve kernel + bitmap by setting their bits back to USED (1)
        bitmap[page / 8] |= (1 << (page % 8));
    }
    
    // Also protect page 0 (frequently contains BIOS data / real-mode structures)
    bitmap[0] |= 1;
}

void *alloc_page(void) {
    // Loop through every byte in the bitmap
    for (uint32_t byte_idx = 0; byte_idx < bitmap_size_bytes; byte_idx++) {
        
        // Optimization: If the byte is 0xFF, all 8 pages in this byte are full. Skip it.
        if (bitmap[byte_idx] == 0xFF) {
            continue;
        }

        // Loop through the 8 bits of this specific byte
        for (int bit_idx = 0; bit_idx < 8; bit_idx++) {
            
            // Check if the bit is 0 (Free)
            if ((bitmap[byte_idx] & (1 << bit_idx)) == 0) {
                
                // Calculate the global page index
                uint32_t page_index = (byte_idx * 8) + bit_idx;
                
                // Safety check to ensure we don't allocate past physical RAM limits
                if (page_index >= total_pages) {
                    return NULL; 
                }

                // Mark the page as USED (1)
                bitmap[byte_idx] |= (1 << bit_idx);

                // Convert the page index back into a raw physical address
                uint32_t physical_address = page_index * PAGE_SIZE;
                return (void *)physical_address;
            }
        }
    }

    return NULL; // Out of memory (OOM)
}

/**
 * Marks a 4KB physical page as free, making it available for future allocations.
 */
void free_page(void *ptr) {
    uint32_t physical_address = (uint32_t)ptr;

    // Alignment safety check: Addresses must be multiples of 4096
    if (physical_address % PAGE_SIZE != 0) {
        return; 
    }

    // Determine which page index this address maps to
    uint32_t page_index = physical_address / PAGE_SIZE;

    // Prevent out-of-bounds corruption
    if (page_index >= total_pages) {
        return; 
    }

    // Clear the bit back to FREE (0)
    bitmap[page_index / 8] &= ~(1 << (page_index % 8));
}
