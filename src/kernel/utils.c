#include <stdint.h>
#include <stdbool.h>
#include "headers/lvga.h"

void lprintk(const char *message) {
	terminal_writestring("[KERNEL] ");
	terminal_writestring(message);
}

void lprintkl(const char *message) {
	terminal_writestring("[KERNEL] ");
	terminal_writestring(message);
	terminal_putchar('\n');
}
