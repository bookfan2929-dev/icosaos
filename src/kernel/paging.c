#include <stdint.h>
#include <stddef.h>
#include "headers/handler.h"
#include "headers/memory.h"

extern void load_page_directory(uint32_t directory_address);

#define PAGE_SIZE 4096

// External reference to your bitmap allocator function
extern void *alloc_page(void);

// Attributes for Page Directory and Page Table entries
#define PAGE_PRESENT  0x1  // Bit 0: Page is in memory
#define PAGE_WRITE    0x2  // Bit 1: Read/Write enabled (0 = Read-only)
#define PAGE_USER     0x4  // Bit 2: User-mode accessible (0 = Supervisor only)

// Explicitly align the master directory to a 4KB boundary
__attribute__((aligned(4096))) uint32_t page_directory[1024];

void init_paging(void) {
    // 1. Clear the entire Page Directory (mark all entries as "not present")
    for (int i = 0; i < 1024; i++) {
        page_directory[i] = 0; 
    }

    // 2. Allocate a 4KB page from your bitmap to act as our first Page Table
    // This table will manage the lowest 4MB of virtual memory (0x00000000 to 0x003FFFFF)
    uint32_t *first_page_table = (uint32_t *)alloc_page();
    if (!first_page_table) {
        // Handle critical Out-of-Memory error
        panic("Out of memory during paging initialization");
    }

    // 3. Identity map the first 4MB page-by-page
    for (uint32_t i = 0; i < 1024; i++) {
        uint32_t physical_address = i * PAGE_SIZE;
        
        // Link the entry to the physical address and set Present + Writable flags
        first_page_table[i] = physical_address | PAGE_PRESENT | PAGE_WRITE;
    }

    // 4. Place our first Page Table into the very first slot of the Page Directory
    page_directory[0] = ((uint32_t)first_page_table) | PAGE_PRESENT | PAGE_WRITE;

    // 5. Hand the Page Directory over to the CPU and enable the paging bit
    load_page_directory((uint32_t)page_directory);
}

int map_page(void *virtual_addr, void *physical_addr, uint32_t flags) {
    uint32_t virt = (uint32_t)virtual_addr;
    uint32_t phys = (uint32_t)physical_addr;

    // 1. Extract the indices from the virtual address
    uint32_t pd_index = virt >> 22;          // Top 10 bits
    uint32_t pt_index = (virt >> 12) & 0x3FF; // Middle 10 bits

    uint32_t *page_table = NULL;

    // 2. Check if a Page Table already exists for this directory slot
    if ((page_directory[pd_index] & PAGE_PRESENT) != 0) {
        // Clear the attribute bits to get the raw physical address of the Page Table
        page_table = (uint32_t *)(page_directory[pd_index] & ~0xFFF);
    } else {
        // The Page Table does not exist yet! We must allocate a new 4KB page for it.
        uint32_t *new_pt = (uint32_t *)alloc_page();
        if (!new_pt) {
            return 0; // Out of memory! Failed to allocate management structures.
        }

        // Clear the newly allocated page table so all its entries start as "not present"
        for (int i = 0; i < 1024; i++) {
            new_pt[i] = 0;
        }

        // Link this new Page Table into our Master Page Directory
        // We use PAGE_PRESENT | PAGE_WRITE so the CPU is allowed to traverse it
        page_directory[pd_index] = ((uint32_t)new_pt) | PAGE_PRESENT | PAGE_WRITE;
        
        page_table = new_pt;
    }

    // 3. Insert the physical address and flags into the target Page Table entry
    // Mask the physical address with flags (ensuring the address is 4KB aligned)
    page_table[pt_index] = (phys & ~0xFFF) | flags;

    // 4. Critical Step: Inform the CPU that the mapping changed!
    // The CPU caches translation mappings in the TLB. We must flush the cached entry.
    __asm__ volatile("invlpg (%0)" : : "r" (virt) : "memory");

    return 1; // Success!
}
