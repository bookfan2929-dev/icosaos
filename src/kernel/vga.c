// CODE BASED OFF OF CODE FROM OSDEV WIKI (https://wiki.osdev.org/Bare_Bones)

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "headers/vga.h"

static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) {
	return fg | bg << 4;
}

static inline uint16_t vga_entry(unsigned char uc, uint8_t color) {
	return (uint16_t) uc | (uint16_t) color << 8;
}

size_t strlen(const char* str) {
	size_t len = 0;
	while (str[len])
		len++;
	return len;
}


size_t terminal_row;
size_t terminal_column;
uint8_t terminal_color;
uint16_t* terminal_buffer = (uint16_t*)VGA_MEMORY;

void terminal_flush(void) {
	terminal_row = 0;
	terminal_column = 0;
	for (int i = 0; i < (VGA_WIDTH * VGA_HEIGHT); i++) {
		terminal_putchar(' ');
	}
	terminal_row = 0;
	terminal_column = 0;
	return;
}

void terminal_initialize(void) {
	terminal_row = 0;
	terminal_column = 0;
	terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
	
	for (size_t y = 0; y < VGA_HEIGHT; y++) {
		for (size_t x = 0; x < VGA_WIDTH; x++) {
			const size_t index = y * VGA_WIDTH + x;
			terminal_buffer[index] = vga_entry(' ', terminal_color);
		}
	}
}

void terminal_setcolor(uint8_t color) {
	terminal_color = color;
}

void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) {
	const size_t index = y * VGA_WIDTH + x;
	terminal_buffer[index] = vga_entry(c, color);
}

void terminal_putchar(char c) {
	if (c == '\n') {
		terminal_row++;
		terminal_column = 0;
		return;
	} else if (c == '\b') {
		terminal_column--;
		terminal_putentryat(' ', terminal_color, terminal_column, terminal_row);
		return;
	} 
	terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
	if (++terminal_column == VGA_WIDTH) {
		terminal_column = 0;
		if (++terminal_row == VGA_HEIGHT) {terminal_row = 0;}
	}
}

void terminal_write(const char* data, size_t size) {
	for (size_t i = 0; i < size; i++)
		terminal_putchar(data[i]);
}

void terminal_writestring(const char* data) {
	terminal_write(data, strlen(data));
}

void terminal_write_hex(unsigned int value) {
	char hex_chars[] = "0123456789ABCDEF";

	char buffer[11];

	buffer[0] = '0';
	buffer[1] = 'x';

	for (int i = 0; i < 8; i++) {
		buffer[9 - i] = hex_chars[(value >> (i *4)) & 0xF];
	}
	buffer[10] = '\0';

	terminal_writestring(buffer);
}

void print_register(const char* name, unsigned int val) {
	terminal_writestring(name);
	terminal_writestring(": ");
	terminal_write_hex(val);
	terminal_writestring("   ");
}
