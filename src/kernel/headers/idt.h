#ifndef IDT_H
#define IDT_H

#include <stdint.h>

#define IDT_ENTRIES 256

typedef struct {
	uint16_t limit;
	uint32_t base;
} __attribute__((packed)) idt_register_t;

// Declare global array and pointer structure

extern idt_register_t idtr;

// Assembly function declaration to actually load the IDT
extern void idt_flush(uint32_t idtr_address);

// High-level initialization routine
void idt_init(void);

#define KERNEL_CODE_SELECTOR 0x08 // byte offset of kernel code segment in GDT

#define IDT_GATE_INT32       0x0E
#define INT_GATE_TRAP32      0x0F

#define INT_DPL_RING0        0x00
#define INT_DPL_RING3        0x03

typedef struct {
	uint16_t offset_low;
	uint16_t selector;
	uint8_t zero;
	uint8_t type_attributes;
	uint16_t offset_high;
} idt_entry_t;

extern idt_entry_t idt[IDT_ENTRIES];

void idt_set_gate(uint8_t vector, uint32_t isr_address, uint8_t attributes);
uint8_t idt_create_attributes(uint8_t gate_type, uint8_t dpl);

#endif
