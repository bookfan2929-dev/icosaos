#ifndef KLOG_H
#define KLOG_H

#include <stdint.h>

// Log status types
typedef enum {
    LOG_SUCCESS,
    LOG_FAILED,
    LOG_WARNING
} log_status_t;

// Standard status colors (assuming 24/32-bit RGB: 0x00RRGGBB)
#define COLOR_WHITE  0x00FFFFFF
#define COLOR_BLACK  0x00000000
#define COLOR_GREEN  0x0000FF00
#define COLOR_RED    0x00FF0000
#define COLOR_YELLOW 0x00FFFF00

// Initialize the logging system
void klog_init(uint32_t fg_color, uint32_t bg_color);

// Step 1: Print the boot message/action (e.g., "Initializing ACPI...")
void klog_info(const char *msg);

// Step 2: Complete the current line with a status bracket on the right
void klog_status(log_status_t status);

// Helper to do both at once if a task is instantly finished
void klog_instant(const char *msg, log_status_t status);

#endif
