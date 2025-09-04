#include <stdarg.h>
#include "my_console.h"

/**
 * @brief Print a simple message to the console.
 *
 * This function wraps Zephyr's printk to print a string followed
 * by a newline. It is useful for quick debugging messages.
 *
 * @param msg Pointer to the null-terminated string message.
 *
 * @return void
 */
void my_console_print(const char *msg)
{
	printk("%s\n", msg);
}

/**
 * @brief Print a formatted message to the console.
 *
 * This function provides printf-style formatted output. It uses
 * Zephyr's vprintf internally to handle variable arguments.
 *
 * @param fmt Format string (printf-style).
 * @param ... Variable arguments for the format string.
 *
 * @return void
 */
void my_console_printf(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
}

