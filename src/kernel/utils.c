#include <stdint.h>
#include <stdbool.h>
#include "headers/vga.h"

void printk(const char *message) {
	terminal_writestring("[KERNEL] ");
	terminal_writestring(message);
}

void printkl(const char *message) {
	terminal_writestring("[KERNEL] ");
	terminal_writestring(message);
	terminal_putchar('\n');
}
