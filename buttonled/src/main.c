#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include "my_console.h"
//#include <zephyr/logging/log.h>

//LOG_MODULE_REGISTER(main);

static const struct gpio_dt_spec led =GPIO_DT_SPEC_GET(DT_ALIAS(led0),gpios);
static const struct gpio_dt_spec button=GPIO_DT_SPEC_GET(DT_ALIAS(sw0),gpios);

static struct gpio_callback button_cb_data;
static bool led_state=false;

void button_pressed(const struct device *dev,struct gpio_callback *cb,uint32_t pins)
{
	if(led_state)
	{
		gpio_pin_set_dt(&led,0);
		led_state=false;
		my_console_print("LED OFF");
	}
	else
	{
		gpio_pin_set_dt(&led,1);
		my_console_printf("LED ON at %d pin\n",led.pin);
		led_state=true;
	}	
}
int main()
{
	if(!device_is_ready(led.port))
	{
		my_console_print("led failed");
		return 0;
	}
	gpio_pin_configure_dt(&led,GPIO_OUTPUT_INACTIVE);
	if(!device_is_ready(button.port))
	{
		my_console_print("button failed");
		return 0;
	}
	gpio_pin_configure_dt(&button,GPIO_INPUT);
	gpio_pin_interrupt_configure_dt(&button,GPIO_INT_EDGE_TO_ACTIVE);

	gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
	gpio_add_callback(button.port,&button_cb_data);
	while(1)
	{
		k_msleep(100);
	}
}
