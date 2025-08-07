#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/sys/printk.h>
static const struct pwm_dt_spec pwm_led0 = PWM_DT_SPEC_GET(DT_ALIAS(pwm_led0));
static const struct pwm_dt_spec pwm_led1 = PWM_DT_SPEC_GET(DT_ALIAS(pwm_led1));

#define STEP_DELAY_MS 200
#define STEP_COUNT 10
#define PWM_PERIOD_NSEC 1000000 //pwm works at nanoseconds
#define MAX_PERIOD PWM_SEC(1U)

int main()
{
	if(!pwm_is_ready_dt(&pwm_led0))
	{
		printk("PWM device %s not ready\n",pwm_led0.dev->name);
	}

	if(!pwm_is_ready_dt(&pwm_led1))
	{
		printk("PWM device %s not ready\n",pwm_led1.dev->name);
	}
	pwm_set_dt(&pwm_led0,MAX_PERIOD,MAX_PERIOD/2U);
	k_msleep(3000);
	pwm_set_dt(&pwm_led1,MAX_PERIOD,MAX_PERIOD/2U);
	printk("max period : %llu\r\n",MAX_PERIOD);
	k_msleep(3000);
	while(1)
	{
		for(int32_t i=0;i<=STEP_COUNT;i++)
		{
			uint32_t pulse0=(PWM_PERIOD_NSEC * i)/STEP_COUNT;
			uint32_t pulse1=(PWM_PERIOD_NSEC * (STEP_COUNT-i))/STEP_COUNT;
			printk("p1:%d\n",pulse0);
			printk("p2:%d\n",pulse1);
			pwm_set_dt(&pwm_led0, PWM_PERIOD_NSEC,pulse0);
			pwm_set_dt(&pwm_led1, PWM_PERIOD_NSEC,pulse1);
			k_msleep(STEP_DELAY_MS);
		}
		for(int32_t i=STEP_COUNT;i>=0;i--)
		{
			uint32_t pulse0=(PWM_PERIOD_NSEC * i)/STEP_COUNT;
			uint32_t pulse1=(PWM_PERIOD_NSEC * (STEP_COUNT-i))/STEP_COUNT;
			printk("rev p1:%d\n",pulse0);
			printk("rev p2:%d\n",pulse1);
			pwm_set_dt(&pwm_led0, PWM_PERIOD_NSEC,pulse0);
			pwm_set_dt(&pwm_led1, PWM_PERIOD_NSEC,pulse1);
			k_msleep(STEP_DELAY_MS);
		}

	}
}

