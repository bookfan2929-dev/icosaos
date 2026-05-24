#include "headers/idt.h"
#include "headers/handler.h"

// Import all 32 distinct exception stubs from assembly
extern void isr_err_0(void);  extern void isr_err_1(void);  extern void isr_err_2(void);  extern void isr_err_3(void);
extern void isr_err_4(void);  extern void isr_err_5(void);  extern void isr_err_6(void);  extern void isr_err_7(void);
extern void isr_err_8(void);  extern void isr_err_9(void);  extern void isr_err_10(void); extern void isr_err_11(void);
extern void isr_err_12(void); extern void isr_err_13(void); extern void isr_err_14(void); extern void isr_err_15(void);
extern void isr_err_16(void); extern void isr_err_17(void); extern void isr_err_18(void); extern void isr_err_19(void);
extern void isr_err_20(void); extern void isr_err_21(void); extern void isr_err_22(void); extern void isr_err_23(void);
extern void isr_err_24(void); extern void isr_err_25(void); extern void isr_err_26(void); extern void isr_err_27(void);
extern void isr_err_28(void); extern void isr_err_29(void); extern void isr_err_30(void); extern void isr_err_31(void);

// Hardware and user stubs
extern void keyboard_gate(void);
extern void syscall_isr_wrapper(void);
extern void timer_interrupt_handler(void);

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

    // 2. Map all 32 Intel architecture exceptions
    idt_set_gate(0,  (uint32_t)isr_err_0,  idt_create_attributes(IDT_GATE_INT32, INT_DPL_RING0));
    idt_set_gate(1,  (uint32_t)isr_err_1,  idt_create_attributes(IDT_GATE_INT32, INT_DPL_RING0));
    idt_set_gate(2,  (uint32_t)isr_err_2,  idt_create_attributes(IDT_GATE_INT32, INT_DPL_RING0));
    idt_set_gate(3,  (uint32_t)isr_err_3,  idt_create_attributes(IDT_GATE_INT32, INT_DPL_RING0));
    idt_set_gate(4,  (uint32_t)isr_err_4,  idt_create_attributes(IDT_GATE_INT32, INT_DPL_RING0));
    idt_set_gate(5,  (uint32_t)isr_err_5,  idt_create_attributes(IDT_GATE_INT32, INT_DPL_RING0));
    idt_set_gate(6,  (uint32_t)isr_err_6,  idt_create_attributes(IDT_GATE_INT32, INT_DPL_RING0));
    idt_set_gate(7,  (uint32_t)isr_err_7,  idt_create_attributes(IDT_GATE_INT32, INT_DPL_RING0));
    idt_set_gate(8,  (uint32_t)isr_err_8,  idt_create_attributes(IDT_GATE_INT32, INT_DPL_RING0));
    idt_set_gate(9,  (uint32_t)isr_err_9,  idt_create_attributes(IDT_GATE_INT32, INT_DPL_RING0));
    idt_set_gate(10, (uint32_t)isr_err_10, idt_create_attributes(IDT_GATE_INT32, INT_DPL_RING0));
    idt_set_gate(11, (uint32_t)isr_err_11, idt_create_attributes(IDT_GATE_INT32, INT_DPL_RING0));
    idt_set_gate(12, (uint32_t)isr_err_12, idt_create_attributes(IDT_GATE_INT32, INT_DPL_RING0));
    idt_set_gate(13, (uint32_t)isr_err_13, idt_create_attributes(IDT_GATE_INT32, INT_DPL_RING0));
    idt_set_gate(14, (uint32_t)isr_err_14, idt_create_attributes(IDT_GATE_INT32, INT_DPL_RING0));
    idt_set_gate(15, (uint32_t)isr_err_15, idt_create_attributes(IDT_GATE_INT32, INT_DPL_RING0));
    idt_set_gate(16, (uint32_t)isr_err_16, idt_create_attributes(IDT_GATE_INT32, INT_DPL_RING0));
    idt_set_gate(17, (uint32_t)isr_err_17, idt_create_attributes(IDT_GATE_INT32, INT_DPL_RING0));
    idt_set_gate(18, (uint32_t)isr_err_18, idt_create_attributes(IDT_GATE_INT32, INT_DPL_RING0));
    idt_set_gate(19, (uint32_t)isr_err_19, idt_create_attributes(IDT_GATE_INT32, INT_DPL_RING0));
    idt_set_gate(20, (uint32_t)isr_err_20, idt_create_attributes(IDT_GATE_INT32, INT_DPL_RING0));
    idt_set_gate(21, (uint32_t)isr_err_21, idt_create_attributes(IDT_GATE_INT32, INT_DPL_RING0));
    idt_set_gate(22, (uint32_t)isr_err_22, idt_create_attributes(IDT_GATE_INT32, INT_DPL_RING0));
    idt_set_gate(23, (uint32_t)isr_err_23, idt_create_attributes(IDT_GATE_INT32, INT_DPL_RING0));
    idt_set_gate(24, (uint32_t)isr_err_24, idt_create_attributes(IDT_GATE_INT32, INT_DPL_RING0));
    idt_set_gate(25, (uint32_t)isr_err_25, idt_create_attributes(IDT_GATE_INT32, INT_DPL_RING0));
    idt_set_gate(26, (uint32_t)isr_err_26, idt_create_attributes(IDT_GATE_INT32, INT_DPL_RING0));
    idt_set_gate(27, (uint32_t)isr_err_27, idt_create_attributes(IDT_GATE_INT32, INT_DPL_RING0));
    idt_set_gate(28, (uint32_t)isr_err_28, idt_create_attributes(IDT_GATE_INT32, INT_DPL_RING0));
    idt_set_gate(29, (uint32_t)isr_err_29, idt_create_attributes(IDT_GATE_INT32, INT_DPL_RING0));
    idt_set_gate(30, (uint32_t)isr_err_30, idt_create_attributes(IDT_GATE_INT32, INT_DPL_RING0));
    idt_set_gate(31, (uint32_t)isr_err_31, idt_create_attributes(IDT_GATE_INT32, INT_DPL_RING0));

    // Hardware IRQs
    idt_set_gate(32, (uint32_t)timer_interrupt_handler, idt_create_attributes(IDT_GATE_INT32, INT_DPL_RING0));
    idt_set_gate(33, (uint32_t)keyboard_gate, idt_create_attributes(IDT_GATE_INT32, INT_DPL_RING0));

    // System Call Gate (Explicit Ring 3 access allowed)
    idt_set_gate(0x40, (uint32_t)syscall_isr_wrapper, idt_create_attributes(IDT_GATE_INT32, INT_DPL_RING3));
	
    // 3. Call assembly to load the IDTR into the processor
    idt_flush((uint32_t)&idtr);
}
