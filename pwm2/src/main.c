#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/sys/printk.h>

/**
 * @brief PWM device tree specification for LED0.
 */
static const struct pwm_dt_spec pwm_led0 = PWM_DT_SPEC_GET(DT_ALIAS(pwm_led0));

/**
 * @brief PWM device tree specification for LED1.
 */
static const struct pwm_dt_spec pwm_led1 = PWM_DT_SPEC_GET(DT_ALIAS(pwm_led1));

/** @brief Delay in milliseconds between PWM duty cycle steps. */
#define STEP_DELAY_MS   200

/** @brief Number of steps in the PWM duty cycle fade. */
#define STEP_COUNT      10

/** @brief PWM period in nanoseconds (1 ms period). */
#define PWM_PERIOD_NSEC 1000000

/** @brief Maximum PWM period (1 second). */
#define MAX_PERIOD      PWM_SEC(1U)

/**
 * @brief Main entry point of the application.
 *
 * - Checks readiness of PWM devices.  
 * - Configures PWM outputs for two LEDs.  
 * - Alternates duty cycle between LED0 and LED1 to create a fading effect.  
 * - Runs indefinitely with step delays.  
 *
 * @retval 0 Always returns 0 in Zephyr (program runs indefinitely).
 */
int main(void)
{
	if (!pwm_is_ready_dt(&pwm_led0)) {
		printk("PWM device %s not ready\n", pwm_led0.dev->name);
	}

	if (!pwm_is_ready_dt(&pwm_led1)) {
		printk("PWM device %s not ready\n", pwm_led1.dev->name);
	}

	/* Initial PWM setup */
	pwm_set_dt(&pwm_led0, MAX_PERIOD, MAX_PERIOD / 2U);
	k_msleep(3000);
	pwm_set_dt(&pwm_led1, MAX_PERIOD, MAX_PERIOD / 2U);
	printk("max period : %llu\r\n", MAX_PERIOD);
	k_msleep(3000);

	while (1) {
		/* Fade-in/fade-out sequence */
		for (int32_t i = 0; i <= STEP_COUNT; i++) {
			uint32_t pulse0 = (PWM_PERIOD_NSEC * i) / STEP_COUNT;
			uint32_t pulse1 = (PWM_PERIOD_NSEC * (STEP_COUNT - i)) / STEP_COUNT;
			printk("p1:%d\n", pulse0);
			printk("p2:%d\n", pulse1);
			pwm_set_dt(&pwm_led0, PWM_PERIOD_NSEC, pulse0);
			pwm_set_dt(&pwm_led1, PWM_PERIOD_NSEC, pulse1);
			k_msleep(STEP_DELAY_MS);
		}

		for (int32_t i = STEP_COUNT; i >= 0; i--) {
			uint32_t pulse0 = (PWM_PERIOD_NSEC * i) / STEP_COUNT;
			uint32_t pulse1 = (PWM_PERIOD_NSEC * (STEP_COUNT - i)) / STEP_COUNT;
			printk("rev p1:%d\n", pulse0);
			printk("rev p2:%d\n", pulse1);
			pwm_set_dt(&pwm_led0, PWM_PERIOD_NSEC, pulse0);
			pwm_set_dt(&pwm_led1, PWM_PERIOD_NSEC, pulse1);
			k_msleep(STEP_DELAY_MS);
		}
	}
}

