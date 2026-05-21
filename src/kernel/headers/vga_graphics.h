#ifndef VGA_GRAPHICS_H
#define VGA_GRAPHICS_H

#include <stdint.h>

// Initializes the text driver with parameters extracted from Multiboot2
void vga_init(void *fb_addr, uint32_t w, uint32_t h, uint32_t pitch_bytes, uint8_t bpp);

// Clears the display with a specific hex color
void vga_clear_screen(uint32_t color);

// Prints a single character handling grid alignment and control characters
void vga_put_char(char c, uint32_t fg_color, uint32_t bg_color);

// Prints a standard null-terminated string to the interface
void vga_print(const char *str, uint32_t fg_color, uint32_t bg_color);

#endif
