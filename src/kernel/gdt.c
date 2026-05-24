#include "headers/gdt.h"
#include <stdint.h>

struct tss_entry_struct {
    uint32_t prev_tss;
    uint32_t esp0;       
    uint32_t ss0;        
    uint32_t esp1;       
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
    uint16_t iomap_base; 
} __attribute__((packed));

typedef struct tss_entry_struct tss_entry_t;

tss_entry_t tss_entry;

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

void write_tss(int32_t num, uint16_t ss0, uint32_t esp0) {
    uint32_t base = (uint32_t) &tss_entry;
    uint32_t limit = sizeof(tss_entry) - 1;

    gdt_set_gate(num, base, limit, 0x89, 0x00);

    for(int i = 0; i < sizeof(tss_entry); i++) {
        ((char*)&tss_entry)[i] = 0;
    }

    tss_entry.ss0 = ss0;   
    tss_entry.esp0 = esp0; 
    tss_entry.iomap_base = sizeof(tss_entry);
}

void flush_tss() {
    asm volatile("ltr %%ax" : : "a" (0x28));
}

extern uint32_t stack_top;

void gdt_init(void) {
    // FIX 2: Change the limit multiplier from 3 to 6 so the CPU tracks the whole table
    gdt_ptr.limit = (sizeof(gdt_entry_t) * 6) - 1;
    gdt_ptr.base = (uint32_t)&gdt;

    // 0: Null segment
    gdt_set_gate(0, 0, 0, 0, 0);

    // 1: Kernel code segment (0x08)
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);

    // 2: Kernel data segment (0x10)
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);

    // 3: User Code Segment (0x1B)
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);

    // 4: User Data Segment (0x23)
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);

    // 5: Task State Segment (0x28)
    // FIX 3: Cast the address of stack_top explicitly to uint32_t
    write_tss(5, 0x10, (uint32_t)&stack_top);

    // Load the GDT descriptor pointer into the CPU
    gdt_flush((uint32_t)&gdt_ptr);

    // Safely load the TSS into the task register now that space exists!
    flush_tss();
}


// TSS code

typedef struct {
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pushed by 'pushad'
    uint32_t gs, fs, es, ds;                         // Data segments
    uint32_t eip, cs, eflags, user_esp, user_ss;     // Pushed automatically by CPU on interrupt
} __attribute__((packed)) cpu_context_t;

typedef struct process {
    int pid;
    uint32_t page_directory;    // CR3 value for virtual memory separation
    uint32_t kernel_stack_top;  // The base of this process's unique kernel stack
    cpu_context_t* context;     // Pointer to the saved registers on the stack
    struct process* next;       // Next process in the ready queue
} process_t;



process_t* current_process;

uint32_t c_scheduler_handler(uint32_t old_esp) {
    // Save the stack pointer to the old process
/*    current_process->context = (cpu_context_t*)old_esp;

    // Pick the next process to run (Simple Round-Robin example)
    current_process = current_process->next;

    // CRUCIAL: Update the TSS so the NEXT interrupt uses this 
    // new process's clean kernel stack, not the old one.
    tss_entry.esp0 = current_process->kernel_stack_top;

    // If you are using paging per-process, update CR3 here:
    // asm volatile("mov %0, %%cr3" : : "r"(current_process->page_directory));
*/
    // Acknowledge the PIC/APIC timer interrupt interrupt here
    // outb(0x20, 0x20);

    // Return the new stack pointer back to the assembly wrapper
    //return (uint32_t)current_process->context;
	return old_esp;
}

