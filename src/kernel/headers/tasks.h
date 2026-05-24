#ifndef TASKS_H
#define TASKS_H

#include <stdint.h>

// 1. Define the context structure completely so kernel.c can use sizeof()
typedef struct {
    // Lowest address space (pushed last in assembly)
    uint32_t gs, fs, es, ds;                         
    uint32_t edi, esi, ebp, esp_at_pushal, ebx, edx, ecx, eax; 
    
    // Highest address space (pushed automatically by CPU hardware)
    uint32_t eip, cs, eflags, user_esp, user_ss;     
} __attribute__((packed)) cpu_context_t;

// 2. Define the process structure completely so kernel.c can access ->kernel_stack_top
typedef struct process {
    int pid;
    uint32_t kernel_stack_top; // Holds the current ESP for this process's kernel stack
} process_t;

// 3. Global process tracking references
extern process_t* current_process;

// 4. Function signatures
process_t* create_user_process(void (*entry_point)(void), uint32_t user_stack_top);
void start_first_process(void);

#endif
