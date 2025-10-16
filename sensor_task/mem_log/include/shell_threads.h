#ifndef SHELL_THREADS_H
#define SHELL_THREADS_H

#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <string.h>
#include "htpg_sensors.h"

/**
 * @file shell_threads.h
 * @brief Sensor thread interface for shell-based applications.
 *
 * This header declares thread entry functions that handle
 * sensor sampling and reporting for humidity/temperature,
 * pressure, and IMU sensors. These threads are typically
 * created and managed in the main application to provide
 * periodic sensor data acquisition.
 */

/**
 * @brief Humidity/temperature sensor thread.
 *
 * Thread entry function for sampling humidity and temperature
 * from the sensor and reporting results (e.g., via logging
 * or shell output).
 *
 * @param a Unused parameter.
 * @param b Unused parameter.
 * @param c Unused parameter.
 *
 * @return void
 */
void hum_thread(void *a, void *b, void *c);

/**
 * @brief Pressure sensor thread.
 *
 * Thread entry function for sampling pressure from the sensor
 * and reporting results (e.g., via logging or shell output).
 *
 * @param a Unused parameter.
 * @param b Unused parameter.
 * @param c Unused parameter.
 *
 * @return void
 */
void press_thread(void *a, void *b, void *c);

/**
 * @brief IMU sensor thread.
 *
 * Thread entry function for sampling IMU (accelerometer
 * and gyroscope) values and reporting results (e.g., via
 * logging or shell output).
 *
 * @param a Unused parameter.
 * @param b Unused parameter.
 * @param c Unused parameter.
 *
 * @return void
 */
void imu_thread(void *a, void *b, void *c);

#endif /* SHELL_THREADS_H */

