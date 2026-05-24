#include "headers/gdt.h"
#include "headers/tss.h"
#include <stdint.h>

extern void gdt_flush(uint32_t);



// FIX 1: Expand the array from 3 to 6 to safely hold your user segments and TSS
gdt_entry_t gdt[6];
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

extern uint32_t stack_top;

void gdt_init(void) {
    gdt_ptr.limit = (sizeof(gdt_entry_t) * 6) - 1;
    gdt_ptr.base = (uint32_t)&gdt;

    gdt_set_gate(0, 0, 0, 0, 0);            // Null
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // Kernel Code
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // Kernel Data
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); // User Code
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); // User Data

    // Setup the TSS structure parameters
    write_tss(0x10, (uint32_t)&stack_top);

    // Map the TSS descriptor into GDT index 5
    uint32_t tss_base = (uint32_t)&tss_entry;
    uint32_t tss_limit = sizeof(tss_entry_t) - 1;
    gdt_set_gate(5, tss_base, tss_limit, 0x89, 0x00);

    // Flush GDT and then load Task Register (0x28 = Index 5)
    gdt_flush((uint32_t)&gdt_ptr);
    flush_tss(0x28); 
}
