#include "headers/idt.h"
#include "headers/handler.h"

extern void exception_gate_32(void);
extern void keyboard_gate(void);

// Allocate memory for 256 descriptors
idt_entry_t idt[IDT_ENTRIES];
idt_register_t idtr;

void idt_set_gate(uint8_t vector, uint32_t isr_address, uint8_t attributes) {
    idt[vector].offset_low = (uint16_t)(isr_address & 0xFFFF);
    idt[vector].selector = KERNEL_CODE_SELECTOR;
    idt[vector].zero = 0;
    idt[vector].type_attributes = attributes;
    idt[vector].offset_high = (uint16_t)((isr_address >> 16) & 0xFFFF);
}

uint8_t idt_create_attributes(uint8_t gate_type, uint8_t dpl) {
    uint8_t attrs = 0;
    attrs |= (1 << 7); // Present bit
    attrs |= ((dpl & 0x03) << 5);
    attrs |= (gate_type & 0x0F);
    return attrs;
}

void idt_init(void) {
    // 1. Set up the IDTR pointer values
    idtr.limit = (sizeof(idt_entry_t) * IDT_ENTRIES) - 1;
    idtr.base  = (uint32_t)&idt;

    // 2. TODO: Initialize all 256 entries to a default stub handler using idt_set_gate
    // Example: idt_set_gate(0, (uint32_t)exception_handler_0, idt_create_attributes(IDT_GATE_INT32, INT_DPL_RING0));

	for (uint8_t i = 0; i < 30; i++) {
		if (i == 15 || (i > 21 && i < 28)) {continue;} 
		idt_set_gate(i, (uint32_t)exception_gate_32, idt_create_attributes(IDT_GATE_INT32, INT_DPL_RING0));
	}

	idt_set_gate(33, (uint32_t)keyboard_gate, idt_create_attributes(IDT_GATE_INT32, INT_DPL_RING0));	
	
    // 3. Call assembly to load the IDTR into the processor
    idt_flush((uint32_t)&idtr);
}

