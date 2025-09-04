#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

/** @brief Delay between LED toggles in milliseconds. */
#define SLEEP_TIMER_MS 500

/** @brief LED1 device tree specification. */
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
/** @brief LED2 device tree specification. */
static const struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);

/**
 * @brief Thread function for toggling LED1.
 *
 * - Runs in an infinite loop.  
 * - Toggles @ref led1 every @ref SLEEP_TIMER_MS milliseconds.  
 *
 * @param a Unused parameter.  
 * @param b Unused parameter.  
 * @param c Unused parameter.  
 *
 * @return void
 */
void led1_thread(void *a, void *b, void *c)
{
	while (1) {
		gpio_pin_toggle_dt(&led1);
		k_msleep(SLEEP_TIMER_MS);
	}
}

/**
 * @brief Thread function for toggling LED2.
 *
 * - Runs in an infinite loop.  
 * - Toggles @ref led2 every @ref SLEEP_TIMER_MS milliseconds.  
 *
 * @param a Unused parameter.  
 * @param b Unused parameter.  
 * @param c Unused parameter.  
 *
 * @return void
 */
void led2_thread(void *a, void *b, void *c)
{
	while (1) {
		gpio_pin_toggle_dt(&led2);
		k_msleep(SLEEP_TIMER_MS);
	}
}

/**
 * @brief Thread definition for LED1 toggling task.
 *
 * - Stack size: 512 bytes  
 * - Priority: 5  
 */
K_THREAD_DEFINE(led1_tid, 512, led1_thread, NULL, NULL, NULL, 5, 0, 0);

/**
 * @brief Thread definition for LED2 toggling task.
 *
 * - Stack size: 512 bytes  
 * - Priority: 5  
 */
K_THREAD_DEFINE(led2_tid, 512, led2_thread, NULL, NULL, NULL, 5, 0, 0);

/**
 * @brief Main entry point of the application.
 *
 * - Checks if LED devices are ready.  
 * - Configures LED GPIO pins as inactive outputs.  
 * - Threads are auto-started by @ref K_THREAD_DEFINE macros.  
 *
 * @retval -1 If LED devices are not ready.  
 * @retval 0  Always returns 0 otherwise (program runs indefinitely).
 */
int main(void)
{
	if (!device_is_ready(led1.port) || !device_is_ready(led2.port)) {
		return -1;
	}

	gpio_pin_configure_dt(&led1, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&led2, GPIO_OUTPUT_INACTIVE);

	// Threads already started by K_THREAD_DEFINE
	// k_thread_start(led1_tid);
	// k_thread_start(led2_tid);

	return 0;
}

