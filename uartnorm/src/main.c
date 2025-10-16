#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include "my_console.h"
#define SLEEP_MS 200
//#define STACKSIZE 1024

static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(DT_ALIAS(led0),gpios);
static const struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(DT_ALIAS(led1),gpios);

const struct device *uart_dev=DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

uint8_t state=0;

K_THREAD_STACK_DEFINE(led_stack,512);
static struct k_thread led_thread_data;

static void leds_config()
{
	gpio_pin_configure_dt(&led1,GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&led2,GPIO_OUTPUT_INACTIVE);
}


void led_thread(void *a, void *b, void *c)
{
	while(1)
	{
		switch(state)
		{
			case 1:
				gpio_pin_set_dt(&led1,1);
				gpio_pin_set_dt(&led2,0);
				break;
			case 2:
				gpio_pin_set_dt(&led1,1);
				gpio_pin_set_dt(&led2,1);
				break;
			case 3:
				gpio_pin_set_dt(&led1,0);
				gpio_pin_set_dt(&led2,0);
				break;
			default:
				gpio_pin_toggle_dt(&led1);
				gpio_pin_toggle_dt(&led2);
				k_msleep(SLEEP_MS);

		}
	}
}

int main()
{
	uint8_t c;
	leds_config();
	 k_thread_create(&led_thread_data, led_stack,K_THREAD_STACK_SIZEOF(led_stack),
                        led_thread, NULL, NULL, NULL,
                        5, 0, K_NO_WAIT);


	while(1)
	{
		if(uart_poll_in(uart_dev,&c)==0)
		{
			switch(c)
			{
				case '1':
					state=1;
					my_console_printf("LED1 ON\r\n");
					break;
				case '2':
					state=2;
					my_console_printf("LED2 ON\r\n");
					break;
				case '3':
					state=3;
					my_console_printf("LED OFF\r\n");
					break;
				default:
					state=4;
					printk("invalid input entered  %.3f\r\n",(float)c);
			}
		}
		k_msleep(10);
	}
	return 0;
}

