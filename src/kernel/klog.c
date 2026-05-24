#include "headers/vga_graphics.h"
#include "headers/klog.h"

// External declaration of our new driver helper
extern uint32_t vga_get_cursor_x(void);

static uint32_t default_fg = COLOR_WHITE;
static uint32_t default_bg = COLOR_BLACK;

#define SCREEN_WIDTH         800 
#define SCALED_FONT_WIDTH    16   // 8 * SCALE_FACTOR (from your scaled driver)
#define STATUS_CHAR_COUNT    8    // Length of "[  OK  ]"

void klog_init(uint32_t fg_color, uint32_t bg_color) {
    default_fg = fg_color;
    default_bg = bg_color;
}

void klog_info(const char *msg) {
    vga_print(msg, default_fg, default_bg);
}

void klog_status(log_status_t status) {
    // 1. Find out where the cursor currently is in pixels
    uint32_t current_x_pixels = vga_get_cursor_x();
    
    // 2. Calculate the pixel position where the status block SHOULD start
    // (Right edge of screen minus the width of the 8-character status block)
    uint32_t status_start_pixel = SCREEN_WIDTH - (STATUS_CHAR_COUNT * SCALED_FONT_WIDTH);
    
    // 3. Fill the gap between the message text and the status block with spaces
    while (current_x_pixels < status_start_pixel) {
        vga_put_char(' ', default_fg, default_bg);
        current_x_pixels = vga_get_cursor_x(); // Update tracking
    }
    
    // 4. Print the status tag right against the edge
    vga_print("[ ", default_fg, default_bg);
    
    if (status == LOG_SUCCESS) {
        vga_print(" OK ", COLOR_GREEN, default_bg);
    } 
    else if (status == LOG_FAILED) {
        vga_print("FAIL", COLOR_RED, default_bg);
    } 
    else if (status == LOG_WARNING) {
        vga_print("WARN", COLOR_YELLOW, default_bg);
    }
    
    vga_print(" ]", default_fg, default_bg);
    
    // 5. CRITICAL: Now that the line is complete, hit Enter to drop to the next line
    vga_put_char('\n', default_fg, default_bg);
}

void klog_instant(const char *msg, log_status_t status) {
    klog_info(msg);
    klog_status(status);
}
