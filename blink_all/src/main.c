#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(DT_ALIAS(led0),gpios);
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(DT_ALIAS(led1),gpios);
static const struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(DT_ALIAS(led2),gpios);

static void config_gpio()
{
	gpio_pin_configure_dt(&led0,GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&led1,GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&led2,GPIO_OUTPUT_INACTIVE);
}

int main()
{
	config_gpio();
	while(1)
	{
		for(uint8_t i=0;i<3;i++)
		{
			gpio_pin_toggle_dt(&led0);
			gpio_pin_toggle_dt(&led1);
			gpio_pin_toggle_dt(&led2);
			k_msleep(1000);
		}

		for(uint8_t i=0;i<3;i++)
		{
			gpio_pin_toggle_dt(&led0);
			k_msleep(500);
			gpio_pin_toggle_dt(&led1);
			k_msleep(500);
			gpio_pin_toggle_dt(&led2);
			k_msleep(500);

		}
	}
}
