#include <stdint.h>
#include <stddef.h>
#include "headers/vga_graphics.h"
#include "headers/sysfont.h" // Contains 'char font8x8_basic[128][8]'

// Base framebuffer byte pointer to bypass strict 32-bit aliasing assumptions
static uint8_t *framebuffer = 0;
static uint32_t fb_width = 0;
static uint32_t fb_height = 0;
static uint32_t fb_pitch_bytes = 0;
static uint8_t  fb_bpp = 0;

// Text terminal positioning counters
static uint32_t cursor_x = 0;
static uint32_t cursor_y = 0;

#define FONT_WIDTH  8
#define FONT_HEIGHT 8

// Low-level helper to write a pixel at an exact X/Y coordinate based on color depth
static inline void vga_write_pixel(uint32_t x, uint32_t y, uint32_t color) {
    uint8_t *row_ptr = framebuffer + (y * fb_pitch_bytes);

    if (fb_bpp == 32) {
        ((uint32_t*)row_ptr)[x] = color;
    } 
    else if (fb_bpp == 24) {
        uint32_t offset = x * 3;
        row_ptr[offset + 0] = (color & 0xFF);         // Blue component
        row_ptr[offset + 1] = ((color >> 8) & 0xFF);  // Green component
        row_ptr[offset + 2] = ((color >> 16) & 0xFF); // Red component
    }
}

// Low-level helper to read a pixel color from a specific coordinate (used for scrolling)
static inline uint32_t vga_read_pixel(uint32_t x, uint32_t y) {
    uint8_t *row_ptr = framebuffer + (y * fb_pitch_bytes);

    if (fb_bpp == 32) {
        return ((uint32_t*)row_ptr)[x];
    } 
    else if (fb_bpp == 24) {
        uint32_t offset = x * 3;
        uint32_t b = row_ptr[offset + 0];
        uint32_t g = row_ptr[offset + 1];
        uint32_t r = row_ptr[offset + 2];
        return b | (g << 8) | (r << 16);
    }
    return 0;
}

void vga_init(void *fb_addr, uint32_t w, uint32_t h, uint32_t pitch_bytes, uint8_t bpp) {
    framebuffer = (uint8_t*)fb_addr;
    fb_width = w;
    fb_height = h;
    fb_pitch_bytes = pitch_bytes;
    fb_bpp = bpp;
    cursor_x = 0;
    cursor_y = 0;
}

void vga_clear_screen(uint32_t color) {
    if (!framebuffer) return;

    for (uint32_t y = 0; y < fb_height; y++) {
        for (uint32_t x = 0; x < fb_width; x++) {
            vga_write_pixel(x, y, color);
        }
    }
}

static void vga_clear_rect(uint32_t x_start, uint32_t y_start, uint32_t w, uint32_t h, uint32_t color) {
    for (uint32_t y = y_start; y < y_start + h && y < fb_height; y++) {
        for (uint32_t x = x_start; x < x_start + w && x < fb_width; x++) {
            vga_write_pixel(x, y, color);
        }
    }
}

static void vga_scroll(uint32_t bg_color) {
    if (fb_height <= FONT_HEIGHT) return;

    uint32_t total_rows_to_copy = fb_height - FONT_HEIGHT;

    // Shift memory buffers upward coordinate-by-coordinate to respect 24-bit alignment limits
    for (uint32_t y = 0; y < total_rows_to_copy; y++) {
        for (uint32_t x = 0; x < fb_width; x++) {
            uint32_t lower_pixel = vga_read_pixel(x, y + FONT_HEIGHT);
            vga_write_pixel(x, y, lower_pixel);
        }
    }

    // Clear out the newly exposed bottom row block cleanly
    vga_clear_rect(0, total_rows_to_copy, fb_width, FONT_HEIGHT, bg_color);
    cursor_y = total_rows_to_copy; 
}

void vga_put_char(char c, uint32_t fg_color, uint32_t bg_color) {
    if (!framebuffer) return;

    // Handle Newline translation
    if (c == '\n') {
        cursor_x = 0;
        cursor_y += FONT_HEIGHT;
        if (cursor_y + FONT_HEIGHT > fb_height) {
            vga_scroll(bg_color);
        }
        return;
    }

    // Handle Carriage Return translation
    if (c == '\r') {
        cursor_x = 0;
        return;
    }

    uint8_t ascii_index = (uint8_t)c;
    if (ascii_index > 127) {
        ascii_index = 0; // Wrap undefined values into standard blank character tiles
    }

    // Process character bitmap row sequences
    for (int row = 0; row < FONT_HEIGHT; row++) {
        uint8_t row_byte = font8x8_basic[ascii_index][row];

        for (int col = 0; col < FONT_WIDTH; col++) {
            // Evaluates font8x8_basic correctly as LSB-first structure layouts
            uint32_t pixel_color = (row_byte & (1 << col)) ? fg_color : bg_color;

            uint32_t target_x = cursor_x + col;
            uint32_t target_y = cursor_y + row;

            if (target_x < fb_width && target_y < fb_height) {
                vga_write_pixel(target_x, target_y, pixel_color);
            }
        }
    }

    // Advance standard pointer spacing metrics
    cursor_x += FONT_WIDTH;

    // Automatic line wrapping constraints check
    if (cursor_x + FONT_WIDTH > fb_width) {
        cursor_x = 0;
        cursor_y += FONT_HEIGHT;

        if (cursor_y + FONT_HEIGHT > fb_height) {
            vga_scroll(bg_color);
        }
    }
}

void vga_print(const char *str, uint32_t fg_color, uint32_t bg_color) {
    while (*str) {
        vga_put_char(*str, fg_color, bg_color);
        str++;
    }
}
