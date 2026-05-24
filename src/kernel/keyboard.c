#include "headers/io.h"
#include "headers/vga_graphics.h"

// Simplified US QWERTY Scan Code Map (Press states only)
static const char scancode_to_ascii[] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
  '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0,  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',   0,
 '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0, '*',   0, ' '
};

#define KEY_BUFFER_SIZE 256

static char key_buffer[KEY_BUFFER_SIZE];
static uint32_t buf_head = 0; // Index where we insert data
static uint32_t buf_tail = 0; // Index where we read data

// Pushes a character into the buffer (Called by Interrupt Handler)
static void enqueue_key(char c) {
    uint32_t next_head = (buf_head + 1) % KEY_BUFFER_SIZE;

    // If the buffer is full, we simply drop the character to prevent overflow
    if (next_head != buf_tail) {
        key_buffer[buf_head] = c;
        buf_head = next_head;
    }
}

// Pulls a character out of the buffer (Called by Shell/Main Loop)
// Returns 0 if no keys are available
char keyboard_get_char(void) {
    if (buf_head == buf_tail) {
        return 0; // Buffer is empty
    }

    char c = key_buffer[buf_tail];
    buf_tail = (buf_tail + 1) % KEY_BUFFER_SIZE;
    return c;
}

// A blocking version that spins until a key is actually pressed
char keyboard_get_char_blocking(void) {
    char c = 0;
    while ((c = keyboard_get_char()) == 0) {
        // You could add an asm("hlt"); here later to save CPU power
        // once you have scheduler/timer ticks set up.
    }
    return c;
}

void keyboard_handler(void) {
    uint8_t scancode = inb(0x60);

    if (!(scancode & 0x80)) {
        if (scancode < sizeof(scancode_to_ascii)) {
            char ascii = scancode_to_ascii[scancode];

            if (ascii != 0) {
                enqueue_key(ascii); // <--- Drop it in the bucket and leave!
            }
        }
    }

    outb(0x20, 0x20); // Send EOI
}


void kernel_read_line(char *buffer, uint32_t max_len) {
    uint32_t index = 0;
    
    // Clear out the buffer safely first
    for (uint32_t i = 0; i < max_len; i++) buffer[i] = 0;

    while (index < max_len - 1) {
        // Wait for a keypress from the circular buffer
        char c = keyboard_get_char_blocking(); 

        // Handle Enter key
        if (c == '\n') {
            vga_put_char('\n', 0xFFFFFFFF, 0x00000000); // Print newline to screen
            break;
        } 
        // Handle Backspace key
        else if (c == '\b') {
            if (index > 0) {
                index--;
                buffer[index] = '\0'; // Remove from memory buffer
                vga_backspace(0x00000000); // Visual backspace on VGA screen
            }
        } 
        // Handle normal printable characters
        else {
            buffer[index] = c;
            index++;
            vga_put_char(c, 0xFFFFFFFF, 0x00000000); // Echo character to VGA screen
        }
    }
    buffer[index] = '\0'; // Guarantee null-termination
}
