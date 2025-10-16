#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include "hd44780_pcf8574.h"
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>


LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

const struct device *hts_dev = DEVICE_DT_GET(DT_ALIAS(ht_sensor));
const struct device *lcd = DEVICE_DT_GET(DT_ALIAS(lcd));

int hum_temp_sensor_lcd_data()
{

	char buf[20];
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
	
	snprintf(buf,sizeof(buf),"Temp: %2.1f C", t);
	hd44780_set_cursor(lcd,0,0);
	hd44780_print(lcd,buf);
	LOG_INF("%s",buf);
	hd44780_set_cursor(lcd,0,1);
	snprintf(buf,sizeof(buf),"Hum:  %2.1f %%", h);
	hd44780_print(lcd,buf);
	LOG_INF("%s",buf);

	return 0;

}

int hum_temp_sensor_check(void)
{
	if (!device_is_ready(hts_dev)) 
	{
	       	LOG_ERR("sensor: %s device not ready.", hts_dev->name);
		hd44780_set_cursor(lcd, 0, 0);
       	 	hd44780_print(lcd, "Sensor err!");

       		return -1;
    	}
	hd44780_set_cursor(lcd, 0, 0);
        hd44780_print(lcd, "Sensor ok!");
	k_msleep(500);


    return 0;
}


int main(void)
{
	
	if (!device_is_ready(lcd)) {
		LOG_ERR("LCD not ready");
		return -1;
	}
	
	hd44780_clear(lcd);
	LOG_INF("clear");
	hum_temp_sensor_check();
	while(1)
	{
		hum_temp_sensor_lcd_data();
		k_msleep(100);
	}
}

