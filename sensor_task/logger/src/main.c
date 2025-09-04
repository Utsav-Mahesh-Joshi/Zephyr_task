#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>

#include "fs_log.h"
#include "shell_cmds.h"

LOG_MODULE_REGISTER(app);
//struct k_thread sampler_t;
//K_THREAD_STACK_DEFINE(sampler_stack,512);
/* devices */
//static const struct device *const dev_hts = DEVICE_DT_GET_ONE(st_hts221);
//static const struct device *const dev_lps = DEVICE_DT_GET_ONE(st_lps22hb_press);
//static const struct device *const dev_imu = DEVICE_DT_GET_ONE(st_lsm6dsl);

static const struct device *const dev_hts = DEVICE_DT_GET(DT_ALIAS(ht_sensor));
static const struct device *const dev_lps = DEVICE_DT_GET(DT_ALIAS(pressure_sensor));
static const struct device *const dev_imu = DEVICE_DT_GET(DT_ALIAS(imu_sensor));

/* last-sample globals (read by shell) */
volatile float g_last_temp_c = 0.0f, g_last_hum = 0.0f, g_last_press_hpa = 0.0f;
volatile float g_last_ax = 0.0f, g_last_ay = 0.0f, g_last_az = 0.0f;

/* runtime controls */
static atomic_t g_live_print = ATOMIC_INIT(0);
//static struct k_mutex g_rate_lock;
//extern uint32_t g_period_ms ;
//g_period_ms = 1000;

void sens_set_live(bool en)
{
	atomic_set(&g_live_print, en ? 1 : 0);
}

/* sampling thread */
void sampler(void *a, void *b, void *c)
{

	while (1) {
		uint32_t period = sens_get_period_ms();
		printk("in thread yo \r\n");
		/* fetch */
		(void)sensor_sample_fetch(dev_hts);
		(void)sensor_sample_fetch(dev_lps);
		(void)sensor_sample_fetch(dev_imu);

		/* HTS221 */
		struct sensor_value t, h;
		if (sensor_channel_get(dev_hts, SENSOR_CHAN_AMBIENT_TEMP, &t) == 0) {
			g_last_temp_c = sensor_value_to_double(&t);
		}
		if (sensor_channel_get(dev_hts, SENSOR_CHAN_HUMIDITY, &h) == 0) {
			g_last_hum = sensor_value_to_double(&h);
		}

		/* LPS22HB */
		struct sensor_value p;
		if (sensor_channel_get(dev_lps, SENSOR_CHAN_PRESS, &p) == 0) {
			g_last_press_hpa = sensor_value_to_double(&p);
		}

		/* LSM6DSL accel */
		struct sensor_value ax, ay, az;
		if (sensor_channel_get(dev_imu, SENSOR_CHAN_ACCEL_X, &ax) == 0 &&
		    sensor_channel_get(dev_imu, SENSOR_CHAN_ACCEL_Y, &ay) == 0 &&
		    sensor_channel_get(dev_imu, SENSOR_CHAN_ACCEL_Z, &az) == 0) {
			g_last_ax = sensor_value_to_double(&ax);
			g_last_ay = sensor_value_to_double(&ay);
			g_last_az = sensor_value_to_double(&az);
		}

		/* CSV line */
		char line[160];
		snprintk(line, sizeof(line), "%llu,%.2f,%.1f,%.2f,%.3f,%.3f,%.3f\r\n",
			(unsigned long long)k_uptime_get(),
			(double)g_last_temp_c, (double)g_last_hum, (double)g_last_press_hpa,
			(double)g_last_ax, (double)g_last_ay, (double)g_last_az);

		(void)fslog_append(line);

		if (atomic_get(&g_live_print)) {
			printk("%s", line);
		}

		/* hint PM: let CPU idle/sleep until next period */
		k_msleep(period);
	}
}

K_THREAD_STACK_DEFINE(sampler_stack, 2048);
static struct k_thread sampler_t;

static int devices_ready(void)
{
	if (!device_is_ready(dev_hts) || !device_is_ready(dev_lps) || !device_is_ready(dev_imu)) {
		LOG_ERR("sensor device(s) not ready");
		return -ENODEV;
	}
	return 0;
}

int main(void)
{
//	k_mutex_init(&g_rate_lock);

	if (devices_ready()) {
		LOG_ERR("Missing sensors; check overlay/board.");
		return 0;
	}

	if (fslog_init()) {
		LOG_ERR("fs init failed");
	}

	/* optional: put unused devices into runtime suspended state later */
//	if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)) {
		/* example: pm_device_action_run(dev_imu, PM_DEVICE_ACTION_SUSPEND); */
//	}

	k_thread_create(&sampler_t, sampler_stack, K_THREAD_STACK_SIZEOF(sampler_stack),
		sampler, NULL, NULL, NULL, K_PRIO_PREEMPT(5), 0, K_NO_WAIT);

	/* shell commands are registered by link */
	LOG_INF("sensor logger ready. try: sens show | sens cat 1024 | sens rate 2000 | sens live on");

	return 0;
}

