
#include "htpg_sensors.h"

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <stdio.h>

LOG_MODULE_REGISTER(sensors);
const struct device *const hts_dev = DEVICE_DT_GET(DT_ALIAS(ht_sensor));
const struct device *const imu_dev = DEVICE_DT_GET(DT_ALIAS(imu_sensor));
const struct device *const pressure_dev = DEVICE_DT_GET(DT_ALIAS(pressure_sensor));

int hum_temp_sensor_get_string(char *buf, size_t buf_len)
{
    int ret;	
    if (!device_is_ready(hts_dev)) return -1;
    if (sensor_sample_fetch(hts_dev) < 0) 
    {
	    return -1;
    }
    struct sensor_value temp, hum;
    if (sensor_channel_get(hts_dev, SENSOR_CHAN_AMBIENT_TEMP, &temp) < 0) return -1;
    if (sensor_channel_get(hts_dev, SENSOR_CHAN_HUMIDITY, &hum) < 0) return -1;

    double t = sensor_value_to_double(&temp);
    double h = sensor_value_to_double(&hum);

    LOG_INF("Temperature: %.1f C", t);
    LOG_INF("Humidity: %.1f %%", h);

    ret=snprintf(buf, buf_len, "Temperature: %.1f C, Humidity: %.1f %%\n", t, h);
    return ret;

}

int hum_temp_sensor_init(void)
{
    if (!device_is_ready(hts_dev)) {
        LOG_ERR("sensor: %s device not ready.", hts_dev->name);
        return -1;
    }


    return 0;
}

int imu_sensor_get_string(char *buf, size_t buf_len)
{
    int ret;
    if (!device_is_ready(imu_dev)) return -1;
    if (sensor_sample_fetch(imu_dev) < 0) 
    {
	    return -1;
    }

    struct sensor_value ax, ay, az, gx, gy, gz;

    sensor_sample_fetch_chan(imu_dev, SENSOR_CHAN_ACCEL_XYZ);
    sensor_channel_get(imu_dev, SENSOR_CHAN_ACCEL_X, &ax);
    sensor_channel_get(imu_dev, SENSOR_CHAN_ACCEL_Y, &ay);
    sensor_channel_get(imu_dev, SENSOR_CHAN_ACCEL_Z, &az);

    sensor_sample_fetch_chan(imu_dev, SENSOR_CHAN_GYRO_XYZ);
    sensor_channel_get(imu_dev, SENSOR_CHAN_GYRO_X, &gx);
    sensor_channel_get(imu_dev, SENSOR_CHAN_GYRO_Y, &gy);
    sensor_channel_get(imu_dev, SENSOR_CHAN_GYRO_Z, &gz);

    double axd = sensor_value_to_double(&ax);
    double ayd = sensor_value_to_double(&ay);
    double azd = sensor_value_to_double(&az);
    double gxd = sensor_value_to_double(&gx);
    double gyd = sensor_value_to_double(&gy);
    double gzd = sensor_value_to_double(&gz);

   LOG_INF("Accel: x=%.2f y=%.2f z=%.2f", axd, ayd, azd);
   LOG_INF("Gyro : x=%.2f y=%.2f z=%.2f", gxd, gyd, gzd);

    ret=snprintf(buf, buf_len,"Accel: %.2f, %.2f, %.2f | Gyro: %.2f, %.2f, %.2f\n",axd, ayd, azd, gxd, gyd, gzd);
    return ret;
}

int imu_sensor_init(void)
{
    if (!device_is_ready(imu_dev)) {
        LOG_ERR("IMU sensor not ready.");
        return -1;
    }
    return 0;
}

int pressure_sensor_get_string(char *buf, size_t buf_len)
{
    int ret;
    if (!device_is_ready(pressure_dev)) return -1;
    if (sensor_sample_fetch(pressure_dev) < 0) 
    {
	    return -1;
    }

    struct sensor_value pressure;
    if (sensor_channel_get(pressure_dev, SENSOR_CHAN_PRESS, &pressure) < 0) return -1;

    double p = sensor_value_to_double(&pressure);
   LOG_INF("Pressure: %.1f kPa", p);

    ret=snprintf(buf, buf_len, "Pressure: %.1f kPa\n", p);
    return ret;
}

int pressure_sensor_init(void)
{
    if (!device_is_ready(pressure_dev))
    {
        LOG_ERR("sensor: %s device not ready.", pressure_dev->name);
        return -1;
    }
    return 0;
}

