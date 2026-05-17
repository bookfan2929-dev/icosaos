#include <stddef.h>
#include <stdint.h>

/**
 * Compares two memory blocks byte by byte.
 * Returns 0 if equal, or the difference between the first mismatched bytes.
 */
int memcmp(const void *s1, const void *s2, size_t n) {
    const uint8_t *p1 = (const uint8_t *)s1;
    const uint8_t *p2 = (const uint8_t *)s2;

    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] - p2[i];
        }
    }
    return 0;
}

/**
 * Copies n bytes from source to destination.
 * Essential for moving data buffers around.
 */
void *memcpy(void *dest, const void *src, size_t n) {
    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;

    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dest;
}

/**
 * Fills a block of memory with a specific byte value.
 * Useful for clearing buffers or structures.
 */
void *memset(void *s, int c, size_t n) {
    uint8_t *p = (uint8_t *)s;

    for (size_t i = 0; i < n; i++) {
        p[i] = (uint8_t)c;
    }
    return s;
}
