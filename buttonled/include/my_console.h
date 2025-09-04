#ifndef MY_CONSOLE_H
#define MY_CONSOLE_H

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

/**
 * @file my_console.h
 * @brief Console printing utility functions for Zephyr.
 *
 * This module provides simple wrapper functions for printing messages
 * to the console using printk. It offers both basic string output
 * and formatted printing similar to printf.
 */

/**
 * @brief Print a message to the console.
 *
 * Wrapper around Zephyr's printk to output a string without formatting.
 *
 * @param msg Pointer to the null-terminated string message.
 *
 * @return void
 */
void my_console_print(const char *msg);

/**
 * @brief Print a formatted message to the console.
 *
 * Wrapper around Zephyr's printk to output formatted text
 * using printf-style formatting.
 *
 * @param fmt Format string (printf-style).
 * @param ... Variable arguments for the format string.
 *
 * @return void
 */
void my_console_printf(const char *fmt, ...);

#endif /* MY_CONSOLE_H */

