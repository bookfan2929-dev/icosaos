#include "headers/tss.h"
#include <stdint.h>

typedef struct process {
    int pid;
    uint32_t kernel_stack_top; // Current ESP inside this process's kernel stack
} process_t;

// Global tracking pointer
process_t* current_process = 0;

// Allocate the actual hardware TSS structure in memory
tss_entry_t tss_entry;
// This function clears the TSS and primes its initial fields
void write_tss(uint16_t ss0, uint32_t esp0) {
    // Clear the memory space
    for (uint32_t i = 0; i < sizeof(tss_entry_t); i++) {
        ((char*)&tss_entry)[i] = 0;
    }

    tss_entry.ss0 = ss0;   // Kernel Data Segment (usually 0x10)
    tss_entry.esp0 = esp0; // Kernel Stack Pointer
    
    // Point beyond the end of the structure to disable I/O port bitmaps
    tss_entry.iomap_base = sizeof(tss_entry_t);
}

// Assembly wrapper to load the Task Register (TR)
void flush_tss(uint16_t tss_selector) {
    // Pass the selector (e.g., 0x28) into AX and load it
    asm volatile("ltr %%ax" : : "a" (tss_selector));
}


