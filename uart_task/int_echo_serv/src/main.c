#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>

//#define EN_BGB   /**< Enable debug logging of each received character. */

/** @brief Register log module for main application. */
LOG_MODULE_REGISTER(main);

/**
 * @brief UART device used as console.
 *
 * This device is bound to `zephyr,console` in the device tree,
 * typically corresponding to the default UART interface.
 */
const struct device *uart_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

/** @brief Input buffer for received UART characters. */
uint8_t instr[128];

/** @brief Index of the current position in the input buffer. */
static int rx_buf;

/**
 * @brief UART interrupt service routine (ISR) callback.
 *
 * Handles UART input using interrupt-driven reception:
 * - Reads characters from the RX FIFO.  
 * - On carriage return (`'\r'`), terminates the string and prints it.  
 * - Handles backspace (`127`) by decrementing buffer index.  
 * - Otherwise, appends the character to @ref instr.  
 *
 * If @ref EN_BGB is defined, debug output is printed for each character
 * showing ASCII value and buffer index.
 *
 * @note No explicit buffer overflow check is implemented — if more than
 * 127 characters are received without pressing Enter, memory corruption
 * may occur.
 */
static void uart_cb(void)
{
	uint8_t c;
	while (uart_irq_update(uart_dev) && uart_irq_is_pending(uart_dev)) {
		if (uart_irq_rx_ready(uart_dev)) {
			uart_fifo_read(uart_dev, &c, 1);

			if (c == '\r') {
				instr[rx_buf] = '\0';
				printk("Recieved : %s\r\n", instr);
				rx_buf = 0;
			} else {
				if (c == 127) {
					if (rx_buf > 0) {
						rx_buf--;
					}
					continue;
				}
				instr[rx_buf++] = c;
#ifdef EN_BGB
				printk(":%d@%d\n", c, rx_buf);
#endif
			}
		}
	}
}

/**
 * @brief Main entry point of the application.
 *
 * - Verifies UART device readiness.  
 * - Registers @ref uart_cb as interrupt callback.  
 * - Enables UART RX interrupts.  
 * - Prints startup message `"Interrupt based echo"`.  
 * - Enters an idle loop with periodic sleep.  
 *
 * Example usage:
 * ```
 * Input : hello<CR>
 * Output: Recieved : hello
 * ```
 *
 * @retval -1 If UART device is not ready.  
 * @retval 0  Always returns 0 otherwise (program runs indefinitely).
 */
int main(void)
{
	if (!device_is_ready(uart_dev)) {
		LOG_WRN("Error in UART");
		return -1;
	}

	uart_irq_callback_user_data_set(uart_dev, uart_cb, NULL);
	uart_irq_rx_enable(uart_dev);

	printk("Interrupt based echo\r\n");

	while (1) {
		k_msleep(100);
	}

	return 0;
}

