#include "headers/vga_graphics.h" 
#include "headers/vga_cosmetics.h"
#include "headers/io.h"

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
