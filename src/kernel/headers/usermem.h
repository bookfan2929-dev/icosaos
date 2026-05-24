#ifndef USERMEM_H
#define USERMEM_H

#include <stdint.h>
#include <stddef.h>

// Standard base addresses for a User-mode process space
#define USER_CODE_BASE     0x40000000  // Where user code/binaries load
#define USER_STACK_MAX     0xBFFFF000  // Top limit of the user stack area
#define USER_STACK_SIZE    (4096 * 4)  // Default allocation size for a user stack (16 KB)

/**
 * Allocates and maps virtual memory pages for a user process.
 * Marks the pages with PAGE_PRESENT | PAGE_WRITE | PAGE_USER.
 * * @param virt_addr Starting virtual address in user space.
 * @param num_pages Number of continuous 4KB pages to allocate.
 * @return 1 on success, 0 on memory/mapping failure.
 */
int allocate_user_space(uint32_t virt_addr, size_t num_pages);

/**
 * Creates, allocates, and maps a dedicated user-mode stack space.
 * The stack grows downward from the returned address.
 * * @return The virtual address representing the TOP of the user stack, or 0 on failure.
 */
uint32_t create_user_stack(void);

/**
 * Safely unmaps and frees the physical memory allocated to a user memory range.
 * * @param virt_addr Starting virtual address to free.
 * @param num_pages Number of pages to release.
 */
void free_user_space(uint32_t virt_addr, size_t num_pages);

#endif
