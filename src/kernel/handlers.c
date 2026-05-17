#include "headers/vga.h"
#include "headers/io.h"
#include "headers/vga.h"

// Struct must perfectly match the assembly stack layout
struct __attribute__((__packed__)) registers_32 {
    unsigned int gs, fs, es, ds;
    unsigned int edi, esi, ebp, esp_at_pushal, ebx, edx, ecx, eax;
    unsigned int eip; // Faulting instruction pointer
    unsigned int cs;
    unsigned int eflags;
};
void exception_handler_32(struct registers_32 *regs) {
    // 1. Force screen initialization to ensure we can write to it
    terminal_initialize();
    
    // 2. Set an alarming color scheme (White text on Red background)
    // VGA color byte format: (background << 4) | foreground
    uint8_t panic_color = (VGA_COLOR_RED << 4) | VGA_COLOR_WHITE;
    terminal_setcolor(panic_color);
    
    // 3. Clear or flush the terminal to apply the background color
    terminal_flush();

    // 4. Print Panic Header
    terminal_writestring("================================================================================\n");
    terminal_writestring("                                  KERNEL PANIC                                  \n");
    terminal_writestring("================================================================================\n\n");
    
    // 5. Print the core culprit (Faulting Instruction Pointer)
    terminal_writestring(" CRITICAL: CPU Exception Triggered!\n");
    terminal_writestring(" Suspected Instruction Pointer (EIP): ");
    terminal_write_hex(regs->eip);
    terminal_writestring("\n\n-------------------------------- REGISTER DUMP ---------------------------------\n");

    // 6. Print general purpose registers
    print_register("EAX", regs->eax);
    print_register("EBX", regs->ebx);
    print_register("ECX", regs->ecx);
    print_register("EDX", regs->edx);
    terminal_writestring("\n");

    print_register("EDI", regs->edi);
    print_register("ESI", regs->esi);
    print_register("EBP", regs->ebp);
    print_register("ESP", regs->esp_at_pushal);
    terminal_writestring("\n");

    // 7. Print system state and segments
    print_register("CS ", regs->cs);
    print_register("DS ", regs->ds);
    print_register("ES ", regs->es);
    print_register("FS ", regs->fs);
    terminal_writestring("\n");

    print_register("GS ", regs->gs);
    print_register("EFLAGS", regs->eflags);
    terminal_writestring("\n--------------------------------------------------------------------------------\n");
    
    terminal_writestring(" System halted safely to prevent data corruption.\n");
    terminal_writestring(" You may now reboot the machine.");

    // 8. Infinite loop to halt execution completely
    while(1) {
        __asm__ __volatile__("cli; hlt");
    }
}

void panic(const char* message) {
	terminal_initialize();
	    
	// 2. Set an alarming color scheme (White text on Red background)
	// VGA color byte format: (background << 4) | foreground
    uint8_t panic_color = (VGA_COLOR_RED << 4) | VGA_COLOR_WHITE;
    terminal_setcolor(panic_color);
    
    // 3. Clear or flush the terminal to apply the background color
    terminal_flush();

    // 4. Print Panic Header
    terminal_writestring("================================================================================\n");
    terminal_writestring("                                  KERNEL PANIC                                  \n");
    terminal_writestring("================================================================================\n\n");
    
    // 5. Print the core culprit (Faulting Instruction Pointer)
    terminal_writestring(" CRITICAL: KERNEL ERROR!\n");
    terminal_writestring(" ERROR MESSAGE:\n ");
    terminal_writestring(message);
    terminal_writestring("\n\n System halted safely to prevent data corruption.\n");
    terminal_writestring(" You may now reboot the machine.");
    // 8. Infinite loop to halt execution completely
    while(1) {
        __asm__ __volatile__("cli; hlt");
    }
	
}







// Simplified US QWERTY Scan Code Map (Press states only)
static const char scancode_to_ascii[] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
  '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0,  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',   0,
 '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0, '*',   0, ' '
};

void keyboard_handler(void) {
    // Read the scan code from the keyboard data port
    uint8_t scancode = inb(0x60);

    // Bit 7 being set means a key was released (break code). 
    // We only care about key presses (make codes) under 0x80 for now.
    if (!(scancode & 0x80)) {
        if (scancode < sizeof(scancode_to_ascii)) {
            char ascii = scancode_to_ascii[scancode];
            
            if (ascii != 0) {
                // You can temporarily print characters directly to test it!
                // Since printk prints a whole message line, you could make a print_char
                // function or modify printk to quickly show it working.
                
                // For an absolute barebones visual test using your printk:
                char str[2] = {ascii, '\0'};
                terminal_putchar(*str);
            }
        }
    }

    // CRITICAL: Send End-Of-Interrupt (EOI) signal back to the PIC.
    // If you don't do this, the PIC will freeze and never send another interrupt.
    outb(0x20, 0x20);
}


