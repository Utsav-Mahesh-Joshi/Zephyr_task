#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include "my_console.h"

#define SLEEP_MS 200  /**< Delay in ms for LED blinking sequence. */

/** @brief LED1 device tree specification. */
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
/** @brief LED2 device tree specification. */
static const struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);

/**
 * @brief UART device used as console.
 *
 * Bound to the device chosen as `zephyr,console` in the device tree.
 */
const struct device *uart_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

/**
 * @brief Configure LEDs as inactive outputs.
 *
 * Initializes @ref led1 and @ref led2 GPIOs so they can be controlled
 * in the main application.
 */
static void leds_config(void)
{
	gpio_pin_configure_dt(&led1, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&led2, GPIO_OUTPUT_INACTIVE);
}

/**
 * @brief Main entry point of the application.
 *
 * - Configures LEDs for output.  
 * - Polls UART input in an infinite loop.  
 * - Based on character received:  
 *   - `'1'`: Turns ON LED1, OFF LED2.  
 *   - `'2'`: Turns ON LED2, OFF LED1.  
 *   - `'3'`: Turns OFF both LEDs.  
 *   - Default: Blinks both LEDs alternately 10 times, then prints received character.  
 *
 * @retval 0 Always returns 0 in Zephyr (program runs indefinitely).
 */
int main(void)
{
	uint8_t c;
	leds_config();

	while (1) {
		if (uart_poll_in(uart_dev, &c) == 0) {
			switch (c) {
			case '1':
				gpio_pin_set_dt(&led1, 1);
				gpio_pin_set_dt(&led2, 0);
				my_console_printf("LED1 ON\r\n");
				break;

			case '2':
				gpio_pin_set_dt(&led1, 0);
				gpio_pin_set_dt(&led2, 1);
				my_console_printf("LED2 ON\r\n");
				break;

			case '3':
				gpio_pin_set_dt(&led1, 0);
				gpio_pin_set_dt(&led2, 0);
				my_console_printf("LED OFF\r\n");
				break;

			default:
				for (int i = 0; i < 10; i++) {
					gpio_pin_toggle_dt(&led1);
					gpio_pin_toggle_dt(&led2);
					k_msleep(SLEEP_MS);
				}
				printk("entered somethin %.3f\r\n", (float)c);
				break;
			}
		}
		k_msleep(10);
	}
	return 0;
}

