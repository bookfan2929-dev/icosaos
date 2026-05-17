#include <stdint.h>
#include <stddef.h>
#include "headers/memory.h"


#define HEAP_START_ADDR 0xD0000000

struct heap_header {
    uint32_t size;              // Size of the data block AFTER this header
    uint8_t is_free;            // 1 if free, 0 if allocated
    struct heap_header *next;   // Pointer to the next block in the chain
};

// Global tracking variables for the heap state
static struct heap_header *heap_start = NULL;
static uint32_t heap_current_end = HEAP_START_ADDR;

/**
 * Requests more 4KB pages from the system to expand the physical heap size.
 */
static int expand_heap(uint32_t bytes_needed) {
    // Round the requested bytes up to the nearest 4KB page boundary
    uint32_t pages_to_alloc = (bytes_needed + sizeof(struct heap_header) + PAGE_SIZE - 1) / PAGE_SIZE;
    
    struct heap_header *new_block = (struct heap_header *)heap_current_end;

    // Allocate and map each required page one by one continuously
    for (uint32_t i = 0; i < pages_to_alloc; i++) {
        void *phys_frame = alloc_page();
        if (!phys_frame) {
            return 0; // Absolute system Out-Of-Memory failure
        }

        // Map the current virtual heap address to our newly found physical page
        map_page((void *)heap_current_end, phys_frame, PAGE_PRESENT | PAGE_WRITE);
        heap_current_end += PAGE_SIZE;
    }

    // Initialize the metadata for this newly mapped block of space
    new_block->size = (pages_to_alloc * PAGE_SIZE) - sizeof(struct heap_header);
    new_block->is_free = 1;
    new_block->next = NULL;

    // Append this new block to the very end of our existing linked list chain
    if (heap_start == NULL) {
        heap_start = new_block;
    } else {
        struct heap_header *current = heap_start;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_block;
    }

    return 1; // Heap expanded successfully
}

/**
 * Kernel Memory Allocator (kmalloc)
 */
void *kmalloc(uint32_t size) {
    if (size == 0) return NULL;

    // Initialize the heap with a base page on the very first allocation call
    if (heap_start == NULL) {
        if (!expand_heap(size)) return NULL;
    }

    struct heap_header *current = heap_start;
    
    // First-Fit Search: Find the first free block that is big enough
    while (current != NULL) {
        if (current->is_free && current->size >= size) {
            
            // Can we split this block? Only split if the leftover space can fit a header + data
            if (current->size >= size + sizeof(struct heap_header) + 4) {
                struct heap_header *split_block = (struct heap_header *)((uint8_t *)current + sizeof(struct heap_header) + size);
                
                split_block->size = current->size - size - sizeof(struct heap_header);
                split_block->is_free = 1;
                split_block->next = current->next;

                current->size = size;
                current->next = split_block;
            }

            current->is_free = 0; // Mark it as taken
            
            // Return a pointer to the data space directly AFTER the header struct
            return (void *)((uint8_t *)current + sizeof(struct heap_header));
        }
        current = current->next;
    }

    // If we reached here, no suitable free block was found. We must grow dynamically!
    if (expand_heap(size)) {
        return kmalloc(size); // Retry allocation now that space exists
    }

    return NULL; // Out of memory
}

/**
 * Kernel Memory Deallocator (kfree)
 */
void kfree(void *ptr) {
    if (!ptr) return;

    // Step backward from the user data pointer to find the start of the block header
    struct heap_header *header = (struct heap_header *)((uint8_t *)ptr - sizeof(struct heap_header));
    header->is_free = 1;

    // Coalescing Loop: Scan list and merge adjacent free blocks to prevent memory fragmentation
    struct heap_header *current = heap_start;
    while (current != NULL && current->next != NULL) {
        if (current->is_free && current->next->is_free) {
            // Merge the next block into the current one
            current->size += sizeof(struct heap_header) + current->next->size;
            current->next = current->next->next;
            // Do not advance 'current' yet, see if the subsequent block can be merged too
        } else {
            current = current->next;
        }
    }
}
