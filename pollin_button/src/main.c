#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#define SLEEP_MS 10  /**< Delay in milliseconds between button reads. */

/**
 * @brief LED device tree specification.
 *
 * Retrieves LED alias from the device tree to control the LED.
 */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

/**
 * @brief Button device tree specification.
 *
 * Retrieves button alias from the device tree to read button state.
 */
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);

/**
 * @brief Main entry point of the application.
 *
 * Configures the LED as output and the button as input.  
 * In the main loop:
 * - Reads button state every @ref SLEEP_MS milliseconds.  
 * - If pressed, turns LED ON; otherwise, turns LED OFF.  
 *
 * @retval -1 If LED or button device is not ready.
 * @retval 0  (Not used in Zephyr bare-metal apps, infinite loop execution).
 */
int main(void)
{
	if (!device_is_ready(led.port) || !device_is_ready(button.port)) {
		return -1;
	}

	gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&button, GPIO_INPUT);

	while (1) {
		bool pressed = gpio_pin_get_dt(&button) == 1;
		gpio_pin_set_dt(&led, pressed ? 1 : 0);
		k_msleep(SLEEP_MS);
	}
}

