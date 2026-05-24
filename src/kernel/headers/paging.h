#ifndef PAGING_H
#define PAGING_H
#include <stdint.h>

#define PAGE_SIZE    4096

#define PAGE_PRESENT 0x1
#define PAGE_WRITE   0x2
#define PAGE_USER    0x4

void init_paging(uint32_t fb_phys_addr, uint32_t fb_pitch_bytes, uint32_t fb_height);
int map_page(void *virtual_addr, void *physical_addr, uint32_t flags);
void  unmap_page(void *virtual_addr);

#endif
