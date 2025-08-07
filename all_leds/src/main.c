#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(my_module,LOG_LEVEL_DBG);
	static const struct gpio_dt_spec led0=GPIO_DT_SPEC_GET(DT_ALIAS(led0),gpios);
	static const struct gpio_dt_spec led1=GPIO_DT_SPEC_GET(DT_ALIAS(led1),gpios);
	static const struct gpio_dt_spec led2=GPIO_DT_SPEC_GET(DT_ALIAS(led2),gpios);

static void leds_config()
{
	gpio_pin_configure_dt(&led0,GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&led1,GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&led2,GPIO_OUTPUT_INACTIVE);
}

int main()
{
	bool state =false;
	leds_config();
	while(1)
	{
		gpio_pin_toggle_dt(&led0);
		gpio_pin_toggle_dt(&led1);
		gpio_pin_toggle_dt(&led2);
		state = !state;
		LOG_DBG("Led is %s\r\n",state ?"ON":"OFF");
		k_msleep(500);
	}



