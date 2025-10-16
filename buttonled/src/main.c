#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include "my_console.h"
//#include <zephyr/logging/log.h>

//LOG_MODULE_REGISTER(main);

/** 
 * @brief LED device tree specification.
 */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

/** 
 * @brief Button device tree specification.
 */
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);

/**
 * @brief Callback structure for button interrupt.
 */
static struct gpio_callback button_cb_data;

/**
 * @brief Current LED state.
 * 
 * `true` if LED is ON, `false` if LED is OFF.
 */
static bool led_state = false;

/**
 * @brief Button press interrupt handler.
 *
 * This function is called whenever the button is pressed.  
 * It toggles the LED state (ON/OFF) and logs the state using
 * `my_console_print()` or `my_console_printf()`.
 *
 * @param dev Pointer to the device structure for the button.
 * @param cb  Pointer to the callback structure.
 * @param pins Bitmask of pins that triggered the interrupt.
 *
 * @return void
 */
void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	if (led_state) {
		gpio_pin_set_dt(&led, 0);
		led_state = false;
		my_console_print("LED OFF");
	} else {
		gpio_pin_set_dt(&led, 1);
		my_console_printf("LED ON at %d pin\n", led.pin);
		led_state = true;
	}
}

/**
 * @brief Main entry point of the application.
 *
 * Initializes the LED and button GPIOs, configures the button
 * for interrupt-on-press, and registers a callback.  
 * The application then enters an infinite loop where it simply 
 * sleeps, waiting for button events.
 *
 * @retval 0 Application executed successfully (though in Zephyr bare-metal apps, this is not used).
 */
int main(void)
{
	if (!device_is_ready(led.port)) {
		my_console_print("led failed");
		return 0;
	}

	gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);

	if (!device_is_ready(button.port)) {
		my_console_print("button failed");
		return 0;
	}

	gpio_pin_configure_dt(&button, GPIO_INPUT);
	gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);

	gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
	gpio_add_callback(button.port, &button_cb_data);

	while (1)
	{
		k_msleep(100);
	}
}

