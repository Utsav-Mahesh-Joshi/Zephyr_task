#ifndef SHELL_THREADS_H
#define SHELL_THREADS_H

#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <string.h>
#include "htpg_sensors.h"
void hum_thread(void *a, void *b, void *c);
void press_thread(void *a, void *b, void *c);
void imu_thread(void *a, void *b, void *c);

#endif
