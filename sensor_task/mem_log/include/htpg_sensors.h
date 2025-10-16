#ifndef HTPG_SENSORS_H
#define HTPG_SENSORS_H

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <stdio.h>
#include <stddef.h>

/**
 * @file htpg_sensors.h
 * @brief Sensor interface for humidity/temperature, IMU, and pressure sensors.
 *
 * This header provides initialization and data retrieval APIs for
 * commonly used environmental and motion sensors in Zephyr-based
 * applications.
 */

/**
 * @brief Initialize humidity and temperature sensor.
 *
 * Probes and configures the humidity/temperature sensor for use.
 *
 * @retval 0 on success.
 * @retval negative error code on failure.
 */
int hum_temp_sensor_init(void);

/**
 * @brief Get humidity and temperature readings as formatted string.
 *
 * Retrieves the latest sensor values and formats them into a
 * null-terminated string stored in @p buf.
 *
 * @param buf      Pointer to buffer for storing the formatted string.
 * @param buf_len  Length of @p buf in bytes.
 *
 * @retval Number of characters written (excluding null terminator).
 * @retval negative error code on failure.
 */
int hum_temp_sensor_get_string(char *buf, size_t buf_len);

/**
 * @brief Initialize IMU sensor.
 *
 * Probes and configures the IMU (accelerome*

