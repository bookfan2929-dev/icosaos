#ifndef USER_HEAP_H
#define USER_HEAP_H

#include <stdint.h>
#include <stddef.h>

/**
 * The starting virtual address for the user-mode dynamic heap allocation zone.
 * Placed well below the kernel's 0xD0000000 boundary to ensure safe isolation.
 */
#define USER_HEAP_START 0x50000000

/**
 * Explicitly packed structure tracking individual memory block metadata on the user heap.
 */
struct __attribute__((__packed__)) user_heap_header {
    uint32_t size;                     // Size of the data block immediately following this header
    uint8_t is_free;                   // 1 if block is available, 0 if allocated to user program
    struct user_heap_header *next;     // Pointer to the next allocation node in the chain
};

/**
 * Allocates a block of memory dynamically from the isolated user heap space.
 * Automatically handles expanding the heap via page tables if more space is needed.
 * * @param size The number of continuous bytes requested by the user application.
 * @return A pointer to the usable data space on success, or NULL if allocation fails.
 */
void *umalloc(size_t size);

/**
 * Frees an allocated block of memory back to the user heap pool.
 * Automatically coalesces adjacent free memory blocks to minimize fragmentation.
 * * @param ptr Pointer to the start of the data area previously returned by umalloc.
 */
void ufree(void *ptr);

#endif
