#ifndef HTPG_SENSORS_H
#define HTPG_SENSORS_H

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <stdio.h>
#include <stddef.h>


int hum_temp_sensor_get_string(char *buf, size_t buf_len);
int hum_temp_sensor_init(void);
int imu_sensor_get_string(char *buf, size_t buf_len);
int imu_sensor_init(void);
int pressure_sensor_get_string(char *buf, size_t buf_len);
int pressure_sensor_init(void);

#endif
