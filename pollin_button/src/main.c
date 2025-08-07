#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#define SLEEP_MS	10

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);

int main()
{
	if (!device_is_ready(led.port) || !device_is_ready(button.port)) 
	{
		return -1;
	}
	gpio_pin_configure_dt(&led,GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&button,GPIO_INPUT);
	while(1)
	{
		bool pressed=gpio_pin_get_dt(&button)==1;
		gpio_pin_set_dt(&led,pressed ?1:0);
		k_msleep(SLEEP_MS);
	}
}
