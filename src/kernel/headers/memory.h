#ifndef MEMORY_H
#define MEMORY_H
#include <stdint.h>
#include "paging.h"
#include "heap.h"
#include "user_heap.h"

void init_bitmap_allocator(uint32_t mbi_addr);
void *alloc_page(void);
void free_page(void *ptr);

#endif
