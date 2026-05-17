#include <stdint.h>

// Assembly I/O port helpers (you might already have these)
void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ( "outb %b0, %w1" : : "a"(val), "Nd"(port) : "memory");
}

uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ( "inb %w1, %b0" : "=a"(ret) : "Nd"(port) : "memory");
    return ret;
}

void init_pic(void) {
    // Save existing interrupt masks
    uint8_t a1 = inb(0x21);
    uint8_t a2 = inb(0xA1);

    // Initialize both Master and Slave PICs (ICW1)
    outb(0x20, 0x11);
    outb(0xA0, 0x11);

    // Remap Master PIC to IDT Vector 32 (0x20)
    outb(0x21, 0x20);
    // Remap Slave PIC to IDT Vector 40 (0x28)
    outb(0xA1, 0x28);

    // Tell Master PIC that Slave PIC is at IRQ2 (0x04)
    outb(0x21, 0x04);
    // Tell Slave PIC its cascade identity (0x02)
    outb(0xA1, 0x02);

    // Put PICs into 8086/88 mode (ICW4)
    outb(0x21, 0x01);
    outb(0xA1, 0x01);

    // Restore masks (or clear them to enable interrupts)
    // For now, let's mask all interrupts EXCEPT IRQ1 (Keyboard)
    outb(0x21, 0xFD); // 0xFD = 11111101b (Bits are inverted: 0 means enabled)
    outb(0xA1, 0xFF); // Disable all on slave PIC
}
