#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>

/**
 * @brief UART device used as console.
 *
 * This device is bound to `zephyr,console` in the device tree,
 * typically corresponding to the default UART interface.
 */
const struct device *uart_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

/**
 * @brief Main entry point of the application.
 *
 * Implements a simple **UART echo server**:
 * - Waits for characters from the UART console.  
 * - Accumulates characters into a buffer (`instr`).  
 * - On carriage return (`'\r'`), null-terminates the string
 *   and prints it back using `printk()`.  
 * - Handles backspace (`127`) by removing the last character.  
 *
 * Example usage:
 * ```
 * Input:  hello<CR>
 * Output: Recieved : hello
 * ```
 *
 * @retval 0 Always returns 0 in Zephyr (program runs indefinitely).
 */
int main(void)
{
	uint8_t c, i = 0;
	uint8_t instr[128];

	printk("Welcome to echo server\r\n");

	while (1) {
		/* Block until a character is available */
		while (uart_poll_in(uart_dev, &c) != 0);

		if (c == '\r') {
			/* End of line: terminate string and echo */
			instr[i] = '\0';
			printk("Recieved : %s\r\n", instr);
			i = 0;
		} else {
			if (c == 127) {
				/* Handle backspace: decrement index */
				if (i > 0) {
					i--;
				}
				continue;
			}
			/* Store character in buffer */
			instr[i++] = c;
			// printk(":%d\n", c);  // Debug raw ASCII code
		}
	}
}

