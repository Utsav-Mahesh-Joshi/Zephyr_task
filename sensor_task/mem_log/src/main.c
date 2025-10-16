#include "fs_log.h"
#include "htpg_sensors.h"
#include "shell_threads.h"

/** @brief Register log module for main application. */
LOG_MODULE_REGISTER(main);

/**
 * @brief Main entry point of the Sensor Shell Logging Demo.
 *
 * - Initializes on-board sensors (HTS221, LPS22HB, LSM6DSL).  
 * - Mounts LittleFS filesystem on the designated flash partition.  
 * - After setup, shell commands (`start_sensors`, `stop_sensors`, `clear_logs`)  
 *   can be used to control periodic sensor logging.  
 *
 * Hardware (on STM32L475 IoT Discovery Kit):
 * - **HTS221**: Humidity and temperature sensor.  
 * - **LPS22HB**: Pressure sensor.  
 * - **LSM6DSL**: 3D accelerometer + 3D gyroscope (IMU).  
 *
 * @retval 0 Always returns 0 in Zephyr (program runs indefinitely via shell).
 */
int main(void)
{
	LOG_INF("Sensor shell logging demo starting...");

	hum_temp_sensor_init();
	pressure_sensor_init();
	imu_sensor_init();

	mount_sens();

	return 0;
}

