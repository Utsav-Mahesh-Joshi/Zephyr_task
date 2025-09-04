#include "fs_log.h"
#include "htpg_sensors.h"
#include "shell_threads.h"
LOG_MODULE_REGISTER(main);

int main()
{
	 LOG_INF("Sensor shell logging demo starting...");

    hum_temp_sensor_init();
    pressure_sensor_init();
    imu_sensor_init();

    mount_sens();
}
