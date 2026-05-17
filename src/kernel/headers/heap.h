#ifndef HEAP_H
#define HEAP_H

#include <stdint.h>

void *kmalloc(uint32_t size);
void kfree(void *ptr);

#endif
