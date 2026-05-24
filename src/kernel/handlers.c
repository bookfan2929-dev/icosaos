#include "headers/vga_graphics.h" 
#include "headers/vga_cosmetics.h"
#include "headers/io.h"
#include "headers/tasks.h"
#include "headers/usermem.h"

#define RGB_WHITE 0xFFFFFF
#define RGB_RED   0xD32F2F

static const char* exception_messages[] = {
    "Division By Zero",
    "Debug",
    "Non-Maskable Interrupt",
    "Breakpoint",
    "Into Detected Overflow",
    "Out of Bounds",
    "Invalid Opcode",
    "No Coprocessor",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Bad TSS",
    "Segment Not Present",
    "Stack-Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",
    "Coprocessor Fault",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Virtualization Exception",
    "Control Protection Exception",
    "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved",
    "Hypervisor Injection Exception",
    "VMM Communication Exception",
    "Security Exception",
    "Reserved"
};

// Struct perfectly matches our uniform macro stack frames
struct __attribute__((__packed__)) registers_32 {
    unsigned int gs, fs, es, ds;                                  // Pushed manually
    unsigned int edi, esi, ebp, esp_at_pushal, ebx, edx, ecx, eax; // Pushed by pushal
    unsigned int interrupt_no, error_code;                        // Pushed by assembly macros
    unsigned int eip, cs, eflags;                                 // Pushed automatically by CPU hardware
};

static void vga_print_hex(uint32_t val) {
    char hex_chars[] = "0123456789ABCDEF";
    char buffer[11];
    buffer[0] = '0';
    buffer[1] = 'x';
    for (int i = 0; i < 8; i++) {
        buffer[2 + i] = hex_chars[(val >> (28 - (i * 4))) & 0xF];
    }
    buffer[10] = '\0';
    vga_print(buffer, RGB_WHITE, RGB_RED);
}

static void print_register(const char* name, uint32_t val) {
    vga_print(" ", RGB_WHITE, RGB_RED);
    vga_print(name, RGB_WHITE, RGB_RED);
    vga_print(": ", RGB_WHITE, RGB_RED);
    vga_print_hex(val); 
    vga_print("  ", RGB_WHITE, RGB_RED);
}

void exception_handler_32(struct registers_32 *regs) {
    vga_clear_screen(RGB_RED);

    vga_print("================================================================================\n", RGB_WHITE, RGB_RED);
    vga_print("                                  KERNEL PANIC                                  \n", RGB_WHITE, RGB_RED);
    vga_print("================================================================================\n\n", RGB_WHITE, RGB_RED);
    
    const char* exception_name = (regs->interrupt_no < 32) ? exception_messages[regs->interrupt_no] : "Unknown Fault";

    vga_print(" CRITICAL: ", RGB_WHITE, RGB_RED);
    vga_print(exception_name, RGB_WHITE, RGB_RED);
    vga_print(" (Vector Vector ", RGB_WHITE, RGB_RED);
    vga_print_hex(regs->interrupt_no);
    vga_print(")\n", RGB_WHITE, RGB_RED);

    vga_print(" Error Code: ", RGB_WHITE, RGB_RED);
    vga_print_hex(regs->error_code);
    vga_print("\n Suspected Instruction Pointer (EIP): ", RGB_WHITE, RGB_RED);
    vga_print_hex(regs->eip);
    vga_print("\n\n-------------------------------- REGISTER DUMP ---------------------------------\n", RGB_WHITE, RGB_RED);

    print_register("EAX", regs->eax);
    print_register("EBX", regs->ebx);
    print_register("ECX", regs->ecx);
    print_register("EDX", regs->edx);
    vga_print("\n", RGB_WHITE, RGB_RED);

    print_register("EDI", regs->edi);
    print_register("ESI", regs->esi);
    print_register("EBP", regs->ebp);
    print_register("ESP", regs->esp_at_pushal);
    vga_print("\n", RGB_WHITE, RGB_RED);

    print_register("CS ", regs->cs);
    print_register("DS ", regs->ds);
    print_register("ES ", regs->es);
    print_register("FS ", regs->fs);
    vga_print("\n", RGB_WHITE, RGB_RED);

    print_register("GS ", regs->gs);
    print_register("EFLAGS", regs->eflags);
    vga_print("\n--------------------------------------------------------------------------------\n", RGB_WHITE, RGB_RED);
    
    vga_print(" System halted safely to prevent data corruption.\n", RGB_WHITE, RGB_RED);
    vga_print(" You may now reboot the machine.", RGB_WHITE, RGB_RED);

    while(1) {
        __asm__ __volatile__("cli; hlt");
    }
}

void panic(const char* message) {
	__asm__ volatile("cli");
    vga_clear_screen(RGB_RED);

    vga_print("================================================================================\n", RGB_WHITE, RGB_RED);
    vga_print("                                  KERNEL PANIC                                  \n", RGB_WHITE, RGB_RED);
    vga_print("================================================================================\n\n", RGB_WHITE, RGB_RED);
    
    vga_print(" CRITICAL: KERNEL ERROR!\n", RGB_WHITE, RGB_RED);
    vga_print(" ERROR MESSAGE:\n  ", RGB_WHITE, RGB_RED);
    vga_print(message, RGB_WHITE, RGB_RED);
    
    vga_print("\n\n System halted safely to prevent data corruption.\n", RGB_WHITE, RGB_RED);
    vga_print(" You may now reboot the machine.", RGB_WHITE, RGB_RED);

    while(1) {
        __asm__ __volatile__("cli; hlt");
    }
}

void c_syscall_handler(void) {
    vga_print("System call received!\n", RGB_WHITE, RGB_RED);
}


#define KERNEL_CS_SELECTOR 0x08
#define USER_CS_SELECTOR   0x1B

void terminate_current_user_process(void) {

//    vga_print("Terminating Process %d cleanly...\n", current_process->pid);
    
    // 1. Free the user space allocations we tracked
    // For now, since your test uses specific hardcoded zones:
    free_user_space(USER_CODE_BASE, 1);
    free_user_space(USER_STACK_MAX - USER_STACK_SIZE, USER_STACK_SIZE / 4096);
    
    // 2. Clear out any allocations it made on your new user heap if applicable
    // (Once your scheduler supports multiple tasks, you would remove this process from the queue)
    
    // 3. Securely halt or loop if there are no other tasks running yet, 
    // instead of letting the CPU execute garbage instructions.
//    printf("System Stable. Standing by.\n");
    
    __asm__ __volatile__("cli");
    while(1) {
        __asm__ __volatile__("hlt");
    }
}
/*
void c_exception_handler(struct registers_32 *regs) {
    // 1. Check if the code segment belongs to the kernel
    // We mask with 0x3 to read the Privilege Level, or check against your exact selector
    if ((regs->cs & 0x3) == 0) {
        // CRITICAL: The kernel faulted! We cannot recover.
        // Enter your newly optimized, fast-printing, CLI-guarded panic block.
        char panic_msg[128];
        sprintf(panic_msg, "Kernel Exception %d (Error Code: 0x%x) at EIP: 0x%x", 
                regs->interrupt_no, regs->error_code, regs->eip);
        panic(panic_msg);
    }
    
    // 2. If we reach here, the exception came from USER SPACE (Ring 3)
    printf("\n[Process %d] SegFault/Exception %d detected at EIP: 0x%x\n", 
           current_process->pid, regs->interrupt_no, regs->eip);
    
    // 3. Handle specific exception types if you want to be fancy later
    if (regs->int_no == 14) { // Page Fault
        uint32_t faulting_address;
        __asm__ __volatile__("mov %%cr2, %0" : "=r" (faulting_address));
        printf(" -> Invalid User Memory Access at Virtual Address: 0x%x\n", faulting_address);
    }
    
    // 4. Terminate the bad process safely instead of halting the computer!
    terminate_current_user_process();
}
*/
