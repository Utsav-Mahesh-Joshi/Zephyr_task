#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#define SLEEP_TIMER_MS 500

static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(DT_ALIAS(led0),gpios);
static const struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(DT_ALIAS(led1),gpios);

void led1_thread(void *a, void *b, void *c)
{
        while(1)
        {
                gpio_pin_toggle_dt(&led1);
                k_msleep(SLEEP_TIMER_MS);
        }
}
void led2_thread(void *a, void *b, void *c)
{
        while(1)
        {
                gpio_pin_toggle_dt(&led2);
                k_msleep(SLEEP_TIMER_MS);
        }
}

K_THREAD_DEFINE(led1_tid,512,led1_thread,NULL,NULL,NULL,5,0,0);
K_THREAD_DEFINE(led2_tid,512,led2_thread,NULL,NULL,NULL,5,0,0);

int main()
{
        if(!device_is_ready(led1.port)||!device_is_ready(led2.port))
        {
                return -1;
        }

        gpio_pin_configure_dt(&led1,GPIO_OUTPUT_INACTIVE);
        gpio_pin_configure_dt(&led2,GPIO_OUTPUT_INACTIVE);

       // k_thread_start(led1_tid);
       // k_thread_start(led2_tid);
}

