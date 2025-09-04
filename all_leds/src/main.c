#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(my_module, LOG_LEVEL_DBG);

/** 
 * @brief GPIO device tree specifications for LEDs.
 * 
 * These structures map the LED aliases defined in the device tree 
 * to gpio_dt_spec objects used for controlling LEDs.
 */
static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
static const struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);

/**
 * @brief Configure LED pins as inactive outputs.
 *
 * This function initializes all LEDs (led0, led1, led2) as GPIO
 * outputs and sets them to inactive state.
 *
 * @return void
 */
static void leds_config(void)
{
	gpio_pin_configure_dt(&led0, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&led1, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&led2, GPIO_OUTPUT_INACTIVE);
}

/**
 * @brief Main application entry point.
 *
 * Initializes LEDs and toggles them in an infinite loop with a delay. 
 * The function logs the LED state (ON/OFF) using Zephyr's logging system.
 *
 * @return int Always returns 0 (not used in Zephyr bare-metal applications).
 */
int main(void)
{
	bool state = false;
	leds_config();

	while (1) {
		gpio_pin_toggle_dt(&led0);
		gpio_pin_toggle_dt(&led1);
		gpio_pin_toggle_dt(&led2);
		state = !state;

		LOG_DBG("Led is %s\r\n", state ? "ON" : "OFF");

		k_msleep(500);
	}
}

