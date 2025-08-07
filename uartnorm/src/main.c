#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include "my_console.h"
#define SLEEP_MS 200
//#define STACKSIZE 1024

static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(DT_ALIAS(led0),gpios);
static const struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(DT_ALIAS(led1),gpios);

const struct device *uart_dev=DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

static void leds_config()
{
	gpio_pin_configure_dt(&led1,GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&led2,GPIO_OUTPUT_INACTIVE);
}

int main()
{
	uint8_t c;
	leds_config();
	while(1)
	{
		if(uart_poll_in(uart_dev,&c)==0)
		{
			switch(c)
			{
				case '1':
					gpio_pin_set_dt(&led1,1);
					gpio_pin_set_dt(&led2,0);
					my_console_printf("LED1 ON\r\n");
					break;
				case '2':
					gpio_pin_set_dt(&led1,0);
					gpio_pin_set_dt(&led2,1);
					my_console_printf("LED2 ON\r\n");
					break;
				case '3':
					gpio_pin_set_dt(&led1,0);
					gpio_pin_set_dt(&led2,0);
					my_console_printf("LED OFF\r\n");
					break;
				default:
					for(int i=0;i<10;i++)
					{
						gpio_pin_toggle_dt(&led1);
						gpio_pin_toggle_dt(&led2);
						k_msleep(SLEEP_MS);
					}
					printk("entered somethin %.3f\r\n",(float)c);
			}
		}
		k_msleep(10);
	}
	return 0;
}
