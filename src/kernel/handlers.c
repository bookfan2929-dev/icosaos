#include "headers/vga_graphics.h" 
#include "headers/io.h"

// 32-bit Hex RGB palette definitions
#define RGB_WHITE 0xFFFFFF
#define RGB_RED   0xD32F2F

// Struct must perfectly match the assembly interrupt stack layout
struct __attribute__((__packed__)) registers_32 {
    unsigned int gs, fs, es, ds;
    unsigned int edi, esi, ebp, esp_at_pushal, ebx, edx, ecx, eax;
    unsigned int eip; // Faulting instruction pointer
    unsigned int cs;
    unsigned int eflags;
};

// Standalone graphic hex printer to safely replace legacy "terminal_write_hex"
static void vga_print_hex(uint32_t val) {
    char hex_chars[] = "0123456789ABCDEF";
    char buffer[11]; // "0x" + 8 digits + null terminator
    
    buffer[0] = '0';
    buffer[1] = 'x';
    
    // Extract nibbles from highest to lowest significance
    for (int i = 0; i < 8; i++) {
        buffer[2 + i] = hex_chars[(val >> (28 - (i * 4))) & 0xF];
    }
    buffer[10] = '\0';
    
    vga_print(buffer, RGB_WHITE, RGB_RED);
}

// Helper to neatly structure register output blocks
static void print_register(const char* name, uint32_t val) {
    vga_print(" ", RGB_WHITE, RGB_RED);
    vga_print(name, RGB_WHITE, RGB_RED);
    vga_print(": ", RGB_WHITE, RGB_RED);
    vga_print_hex(val); 
    vga_print("  ", RGB_WHITE, RGB_RED);
}

void exception_handler_32(struct registers_32 *regs) {
    // 1. Wipe the entire display with the panic red color
    vga_clear_screen(RGB_RED);

    // 2. Output Panic Banner Headers
    vga_print("================================================================================\n", RGB_WHITE, RGB_RED);
    vga_print("                                  KERNEL PANIC                                  \n", RGB_WHITE, RGB_RED);
    vga_print("================================================================================\n\n", RGB_WHITE, RGB_RED);
    
    // 3. Output primary structural exception states
    vga_print(" CRITICAL: CPU Exception Triggered!\n", RGB_WHITE, RGB_RED);
    vga_print(" Suspected Instruction Pointer (EIP): ", RGB_WHITE, RGB_RED);
    vga_print_hex(regs->eip);
    vga_print("\n\n-------------------------------- REGISTER DUMP ---------------------------------\n", RGB_WHITE, RGB_RED);

    // 4. Print general purpose registers
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

    // 5. Print CPU tracking segment blocks
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

	

	
	
    // 6. Hard-lock execution state machine
    while(1) {
        __asm__ __volatile__("cli; hlt");
    }
}

void panic(const char* message) {
    // 1. Wipe out screen spaces using the panic red color
    vga_clear_screen(RGB_RED);

    // 2. Output structural banner frames
    vga_print("================================================================================\n", RGB_WHITE, RGB_RED);
    vga_print("                                  KERNEL PANIC                                  \n", RGB_WHITE, RGB_RED);
    vga_print("================================================================================\n\n", RGB_WHITE, RGB_RED);
    
    // 3. Print out descriptive custom fault message logs
    vga_print(" CRITICAL: KERNEL ERROR!\n", RGB_WHITE, RGB_RED);
    vga_print(" ERROR MESSAGE:\n  ", RGB_WHITE, RGB_RED);
    vga_print(message, RGB_WHITE, RGB_RED);
    
    vga_print("\n\n System halted safely to prevent data corruption.\n", RGB_WHITE, RGB_RED);
    vga_print(" You may now reboot the machine.", RGB_WHITE, RGB_RED);
	

    // 4. Hard-lock execution state machine
    while(1) {
        __asm__ __volatile__("cli; hlt");
    }
}
