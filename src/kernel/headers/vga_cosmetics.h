#ifndef VGA_COSMETICS
#define VGA_COSMETICS

// 32-bit ARGB/XRGB (0x00RRGGBB) VGA Color Palette

// --- Monochromatic ---
#define VGA_BLACK          0x00000000
#define VGA_DARK_GRAY      0x00808080
#define VGA_LIGHT_GRAY     0x00C0C0C0
#define VGA_WHITE          0x00FFFFFF

// --- Reds & Pinks ---
#define VGA_DARK_RED       0x00800000  // Original Red
#define VGA_LIGHT_RED      0x00FF0000  // Original Light Red

// --- Greens ---
#define VGA_DARK_GREEN     0x00008000  // Original Green
#define VGA_LIGHT_GREEN    0x0000FF00  // Original Light Green

// --- Blues ---
#define VGA_DARK_BLUE      0x00000080  // Original Blue
#define VGA_LIGHT_BLUE     0x000000FF  // Original Light Blue

// --- Cyans ---
#define VGA_DARK_CYAN      0x00008080  // Original Cyan
#define VGA_LIGHT_CYAN     0x0000FFFF  // Original Light Cyan

// --- Magentas ---
#define VGA_DARK_MAGENTA   0x00800080  // Original Magenta
#define VGA_LIGHT_MAGENTA  0x00FF00FF  // Original Light Magenta

// --- Yellows & Browns ---
#define VGA_BROWN          0x00808000  // Original Brown (Dark Yellow)
#define VGA_YELLOW         0x00FFFF00  // Original Yellow

// --- Expanded Symmetrical Duplicates (Dark/Light Pairs) ---
#define VGA_EXT_DARK_BROWN 0x00404000  // Darker version of Brown
#define VGA_EXT_DARK_GRAY2 0x00404040  // Charcoal (Darker than Dark Gray)
#define VGA_EXT_NAVY_BLUE  0x00000040  // Ultra-dark Blue
#define VGA_EXT_FOREST     0x00004000  // Ultra-dark Green
#define VGA_EXT_MAROON     0x00400000  // Ultra-dark Red
#define VGA_EXT_DEEP_CYAN  0x00004040  // Ultra-dark Cyan
#define VGA_EXT_PURPLE     0x00400040  // Ultra-dark Magenta
#define VGA_EXT_CREAM      0x00FFFF80  // Pastel/Light version of Yellow


#endif
