#include "headers/usermem.h"
#include "headers/memory.h"

// Define missing page directory/table attributes matching your paging.c definitions
#define PAGE_PRESENT  0x1
#define PAGE_WRITE    0x2
#define PAGE_USER     0x4

int allocate_user_space(uint32_t virt_addr, size_t num_pages) {
    // Basic page boundary alignment safety check
    if (virt_addr % 4096 != 0) {
        return 0;
    }

    uint32_t current_virt = virt_addr;

    for (size_t i = 0; i < num_pages; i++) {
        // 1. Fetch a clean physical page frame from your bitmap allocator
        void *phys_frame = alloc_page();
        if (!phys_frame) {
            // Rollback previously mapped pages if we hit an Out-Of-Memory limit mid-loop
            free_user_space(virt_addr, i);
            return 0;
        }

        // 2. Map the physical address to the requested virtual location.
        // Explicitly set the USER privilege bit (0x4) along with Present & Writable.
        uint32_t flags = PAGE_PRESENT | PAGE_WRITE | PAGE_USER; // 0x7
        if (!map_page((void *)current_virt, phys_frame, flags)) {
            free_page(phys_frame);
            free_user_space(virt_addr, i);
            return 0;
        }

        current_virt += 4096;
    }

    return 1; // Success!
}

uint32_t create_user_stack(void) {
    size_t pages_needed = USER_STACK_SIZE / 4096;
    
    // Calculate the bottom virtual memory starting position for our stack area
    // If the stack size is 16KB, the bottom will sit at 0xBFFFF000 - 16384 = 0xBFFE0000
    uint32_t stack_bottom_virt = USER_STACK_MAX - USER_STACK_SIZE;

    // Allocate and map the stack frame pages as user accessible
    if (!allocate_user_space(stack_bottom_virt, pages_needed)) {
        return 0; // Allocation failure
    }

    // Return USER_STACK_MAX, because stacks grow DOWNWARD on x86. 
    // The very first push will safely subtract from this address limit.
    return USER_STACK_MAX;
}

void free_user_space(uint32_t virt_addr, size_t num_pages) {
    if (virt_addr % 4096 != 0) return;

    uint32_t current_virt = virt_addr;

    for (size_t i = 0; i < num_pages; i++) {
        // On an x86 paging implementation, we must find the physical address 
        // linked to this virtual address so we can return it to the bitmap allocator.
        // We track down the directory indexes manually from your architectural design:
        uint32_t pd_index = current_virt >> 22;
        uint32_t pt_index = (current_virt >> 12) & 0x3FF;

        extern uint32_t page_directory[1024];
        if ((page_directory[pd_index] & PAGE_PRESENT) != 0) {
            uint32_t *page_table = (uint32_t *)(page_directory[pd_index] & ~0xFFF);
            
            if ((page_table[pt_index] & PAGE_PRESENT) != 0) {
                uint32_t phys_addr = page_table[pt_index] & ~0xFFF;
                
                // Return the physical frame to memory.c bitmap tracking
                free_page((void *)phys_addr);
            }
        }

        // Unlink the virtual translation from the directory and flush the TLB cache via invlpg
        unmap_page((void *)current_virt);
        current_virt += 4096;
    }
}
