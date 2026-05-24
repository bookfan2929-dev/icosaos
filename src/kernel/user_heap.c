// user_heap.c
#include <stdint.h>
#include <stddef.h>
#include "headers/usermem.h"

#define USER_HEAP_START 0x50000000

struct user_heap_header {
    uint32_t size;
    uint8_t is_free;
    struct user_heap_header *next;
};

static struct user_heap_header *uheap_start = NULL;
static uint32_t uheap_current_end = USER_HEAP_START;

// Custom user heap expander utilizing PAGE_USER flags
static int expand_user_heap(uint32_t bytes_needed) {
    // Round up to 4KB page boundaries
    uint32_t pages_to_alloc = (bytes_needed + sizeof(struct user_heap_header) + 4096 - 1) / 4096;
    
    uint32_t old_end = uheap_current_end;
    
    // Allocate raw physical frames and map them with USER permissions (0x7)
    if (!allocate_user_space(uheap_current_end, pages_to_alloc)) {
        return 0; // Out of memory
    }
    
    uheap_current_end += (pages_to_alloc * 4096);
    
    struct user_heap_header *new_block = (struct user_heap_header *)old_end;
    new_block->size = (pages_to_alloc * 4096) - sizeof(struct user_heap_header);
    new_block->is_free = 1;
    new_block->next = NULL;
    
    // Append to the chain
    if (uheap_start == NULL) {
        uheap_start = new_block;
    } else {
        struct user_heap_header *current = uheap_start;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_block;
    }
    
    return 1;
}

void *umalloc(size_t size) {
    if (size == 0) return NULL;
    
    if (uheap_start == NULL) {
        if (!expand_user_heap(size)) return NULL;
    }
    
    struct user_heap_header *current = uheap_start;
    while (current != NULL) {
        if (current->is_free && current->size >= size) {
            // Your excellent block splitting logic from heap.c goes here!
            if (current->size >= size + sizeof(struct user_heap_header) + 4) {
                struct user_heap_header *split = (struct user_heap_header *)((uint8_t *)current + sizeof(struct user_heap_header) + size);
                split->size = current->size - size - sizeof(struct user_heap_header);
                split->is_free = 1;
                split->next = current->next;
                
                current->size = size;
                current->next = split;
            }
            current->is_free = 0;
            return (void *)((uint8_t *)current + sizeof(struct user_heap_header));
        }
        current = current->next;
    }
    
    if (expand_user_heap(size)) {
        return umalloc(size);
    }
    
    return NULL;
}

void ufree(void *ptr) {
    if (!ptr) return;
    
    struct user_heap_header *header = (struct user_heap_header *)((uint8_t *)ptr - sizeof(struct user_heap_header));
    header->is_free = 1;
    
    // Coalescing Loop matching your heap.c implementation to prevent fragmentation
    struct user_heap_header *current = uheap_start;
    while (current != NULL && current->next != NULL) {
        if (current->is_free && current->next->is_free) {
            current->size += sizeof(struct user_heap_header) + current->next->size;
            current->next = current->next->next;
        } else {
            current = current->next;
        }
    }
}
