#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

/**
 * @brief Initialization/Interrupt handler for the keyboard.
 * * This function should be registered in your IDT at interrupt vector 33 (IRQ 1).
 * It reads the raw scan code from I/O port 0x60, translates it to ASCII using
 * the internal layout mapping, and pushes the result to the circular buffer.
 */
void keyboard_handler(void);

/**
 * @brief Retrieves a single character from the internal keyboard circular buffer.
 * * This function is non-blocking. If no key has been pressed, it returns 0 immediately.
 * * @return char The ASCII character pressed, or 0 if the buffer is empty.
 */
char keyboard_get_char(void);

/**
 * @brief Retrieves a single character from the keyboard, blocking until one is available.
 * * This function spins in a loop until a key is pressed and a character can be pulled
 * from the circular buffer.
 * * @return char The ASCII character pressed.
 */
char keyboard_get_char_blocking(void);

/**
 * @brief Reads an entire line of input into a buffer, handling backspace and echo.
 * * This is the line discipliner layer. It blocks, reading characters from the keyboard
 * buffer one by one, echoing them to the VGA screen, handling visual and logical backspaces,
 * and returns once a newline ('\n') is received. The string is guaranteed to be null-terminated.
 * * @param buffer Pointer to the character array where the input line will be stored.
 * @param max_len The maximum number of bytes to write into the buffer (including the null terminator).
 */
void kernel_read_line(char *buffer, uint32_t max_len);

#endif // KEYBOARD_H
