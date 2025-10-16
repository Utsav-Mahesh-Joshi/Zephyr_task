#include "fs_log.h"
#include "htpg_sensors.h"
#include "shell_threads.h"
#include <zephyr/drivers/uart.h>
#include <zephyr/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/pm/policy.h>
LOG_MODULE_REGISTER(main);
const struct device *uart_dev=DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

int main()
{
	 LOG_INF("Sensor shell logging demo starting...");
   	 pm_device_runtime_disable(uart_dev);
    	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_RAM ,PM_ALL_SUBSTATES);
    	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_RAM,PM_ALL_SUBSTATES);
    	LOG_INF("power stuff");
    	hum_temp_sensor_init();
    	pressure_sensor_init();
    	imu_sensor_init();

   	 mount_sens();
}
