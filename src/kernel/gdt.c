#include "headers/gdt.h"

#include <stdint.h>

// Ensure the compiler doesn't add padding bytes
struct tss_entry_struct {
    uint32_t prev_tss;   // Unused
    uint32_t esp0;       // The kernel stack pointer to load on Ring 3 -> Ring 0 switches
    uint32_t ss0;        // The kernel stack segment descriptor (usually 0x10)
    uint32_t esp1;       // Unused
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;         
    uint32_t cs;        
    uint32_t ss;        
    uint32_t ds;        
    uint32_t fs;       
    uint32_t gs;         
    uint32_t ldt;        
    uint16_t trap;
    uint16_t iomap_base; // Point this beyond the limit string if not using IO bitmaps
} __attribute__((packed));

typedef struct tss_entry_struct tss_entry_t;

// Define a global instance of your TSS
tss_entry_t tss_entry;

extern void gdt_flush(uint32_t);

gdt_entry_t gdt[3];
gdt_ptr_t gdt_ptr;

static void gdt_set_gate(
    int num,
    uint32_t base,
    uint32_t limit,
    uint8_t access,
    uint8_t gran
) {
    gdt[num].base_low = base & 0xFFFF;
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;

    gdt[num].limit_low = limit & 0xFFFF;

    gdt[num].granularity = (limit >> 16) & 0x0F;
    gdt[num].granularity |= gran & 0xF0;

    gdt[num].access = access;
}

void write_tss(int32_t num, uint16_t ss0, uint32_t esp0) {
    // 1. Calculate the base and limit of our TSS structure
    uint32_t base = (uint32_t) &tss_entry;
    uint32_t limit = sizeof(tss_entry) - 1;

    // 2. Add the TSS descriptor to the GDT
    // Access: 0x89 (Present, Ring 0 DPL, Type: 32-bit Available TSS)
    // Granularity: 0x00 (Byte granularity, since the size is tiny)
    gdt_set_gate(num, base, limit, 0x89, 0x00);

    // 3. Clear the TSS memory space
    for(int i = 0; i < sizeof(tss_entry); i++) {
        ((char*)&tss_entry)[i] = 0;
    }

    // 4. Set up the crucial privilege switch parameters
    tss_entry.ss0 = ss0;   // Your kernel data segment selector (e.g., 0x10)
    tss_entry.esp0 = esp0; // The top of your kernel stack space
    
    // Effectively disables the I/O port bitmap protection check
    tss_entry.iomap_base = sizeof(tss_entry); 
}

void flush_tss() {
    // 0x28 is the byte offset/selector for GDT entry index 5
    asm volatile("ltr %%ax" : : "a" (0x28));
}

extern uint32_t stack_top;

void gdt_init(void) {
    gdt_ptr.limit = (sizeof(gdt_entry_t) * 3) - 1;
    gdt_ptr.base = (uint32_t)&gdt;

    // Null segment
    gdt_set_gate(0, 0, 0, 0, 0);

    // Kernel code segment
    gdt_set_gate(
        1,
        0,
        0xFFFFFFFF,
        0x9A,
        0xCF
    );

    // Kernel data segment
    gdt_set_gate(
        2,
        0,
        0xFFFFFFFF,
        0x92,
        0xCF
    );

		// User Code Segment (Index 3, or whichever index matches your setup)
	// Base: 0, Limit: 0xFFFFF, Access: 0xFA, Granularity: 0xCF
	gdt_set_gate(3, 0, 0xFFFFF, 0xFA, 0xCF);

	// User Data Segment (Index 4)
	// Base: 0, Limit: 0xFFFFF, Access: 0xF2, Granularity: 0xCF
	gdt_set_gate(4, 0, 0xFFFFF, 0xF2, 0xCF);

	write_tss(5, 0x10, stack_top);
    

    gdt_flush((uint32_t)&gdt_ptr);

    flush_tss();
}
