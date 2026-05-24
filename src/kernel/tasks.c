#include <stdint.h>
#include "headers/memory.h"
#include "headers/tasks.h"
#include "headers/io.h" 
#include "headers/tss.h"

// The core scheduler function
uint32_t c_scheduler_handler(uint32_t old_esp) {
    // 1. Send End-of-Interrupt (EOI) to the Master PIC 
    
    outb(0x20, 0x20);

    // 2. Safety check: If we haven't loaded any user processes yet,
    // safely return the exact same stack pointer to avoid a kernel panic.
    if (current_process == 0) {
        return old_esp;
    }

    // 3. Save the current process's stack pointer state
    current_process->kernel_stack_top = old_esp;

    // 4. (Future state) Round-Robin or Priority queue selection:
    // current_process = current_process->next;

    // 5. Update the TSS so the NEXT hardware interrupt lands safely
    // at the base of this process's clean kernel stack.
    tss_entry.esp0 = current_process->kernel_stack_top + sizeof(cpu_context_t);

    // 6. Return the next process's stack pointer back to assembly
    return current_process->kernel_stack_top;
}

// This memory layout must perfectly match what our assembly wrappers pop!

process_t* create_user_process(void (*entry_point)(void), uint32_t user_stack_top) {
    process_t* new_proc = (process_t*)umalloc(sizeof(process_t));
    uint32_t* kstack = (uint32_t*)umalloc(16384); 
    
    // Explicitly calculate the top dword alignment
    uint32_t* esp = (uint32_t*)((uint32_t)kstack + 16384);

	// jus saying this is weird sorcery ahh shi

    // Push elements manually in REVERSE order of how assembly pops them!
    *(--esp) = 0x23;            // user_ss   (User Data, Ring 3 privilege)
    *(--esp) = user_stack_top;  // user_esp  (The Ring 3 stack pointer)
    *(--esp) = 0x202;           // eflags    (Interrupts enabled)
    *(--esp) = 0x1B;            // cs        (User Code, Ring 3 privilege)
    *(--esp) = (uint32_t)entry_point; // eip (Where the program begins)

    // pushal registers (8 dwords)
    *(--esp) = 0;               // eax
    *(--esp) = 0;               // ecx
    *(--esp) = 0;               // edx
    *(--esp) = 0;               // ebx
    *(--esp) = (uint32_t)esp;   // esp_at_pushal
    *(--esp) = 0;               // ebp
    *(--esp) = 0;               // esi
    *(--esp) = 0;               // edi

    // Segment registers
    *(--esp) = 0x23;            // ds
    *(--esp) = 0x23;            // es
    *(--esp) = 0x23;            // fs
    *(--esp) = 0x23;            // gs

    // Save the exact top address where our elements start
    new_proc->kernel_stack_top = (uint32_t)esp;
    new_proc->pid = 1;

    return new_proc;
}
