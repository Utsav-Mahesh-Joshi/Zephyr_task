#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

/** 
 * @brief GPIO device tree specifications for LEDs.
 * 
 * These structures represent the LED aliases defined in the 
 * device tree and are used for GPIO control.
 */
static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
static const struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);

/**
 * @brief Configure LED GPIOs.
 *
 * Sets the LED pins (led0, led1, led2) as outputs
 * and initializes them to inactive state.
 *
 * @return void
 */
static void config_gpio(void)
{
	gpio_pin_configure_dt(&led0, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&led1, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&led2, GPIO_OUTPUT_INACTIVE);
}

/**
 * @brief Main entry point of the application.
 *
 * Initializes the LED GPIOs and runs an infinite loop that:
 * 1. Toggles all three LEDs together three times with 1-second delay.
 * 2. Toggles each LED sequentially with 0.5-second delay between toggles.
 *
 * @return int Always returns 0 (not used in Zephyr bare-metal apps).
 */
int main(void)
{
	config_gpio();

	while (1) {
		/* Step 1: Toggle all LEDs together */
		for (uint8_t i = 0; i < 3; i++) {
			gpio_pin_toggle_dt(&led0);
			gpio_pin_toggle_dt(&led1);
			gpio_pin_toggle_dt(&led2);
			k_msleep(1000);
		}

		/* Step 2: Toggle LEDs sequentially */
		for (uint8_t i = 0; i < 3; i++) {
			gpio_pin_toggle_dt(&led0);
			k_msleep(500);
			gpio_pin_toggle_dt(&led1);
			k_msleep(500);
			gpio_pin_toggle_dt(&led2);
			k_msleep(500);
		}
	}
}

