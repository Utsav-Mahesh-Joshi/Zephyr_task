#ifndef HD44780_PCF8574_PRIV_H
#define HD44780_PCF8574_PRIV_H

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

/*
 * PCF8574 bit mapping (common backpacks):
 * P0=RS, P1=RW, P2=E, P3=BL, P4=D4, P5=D5, P6=D6, P7=D7
 * Backlight is active-high on many boards; some are inverted.
 * We allow DT to flip that via 'bl-active-low'.
 */

#define P_RS	BIT(0)
#define P_RW	BIT(1)
#define P_E	BIT(2)
#define P_BL	BIT(3)
#define P_D4	BIT(4)
#define P_D5	BIT(5)
#define P_D6	BIT(6)
#define P_D7	BIT(7)

struct hd44780_pcf8574_cfg
{
	struct i2c_dt_spec	i2c;
	uint8_t			cols;
	uint8_t			rows;
	bool			bl_active_low;
};

struct hd44780_pcf8574_data
{
	uint8_t			ctrl;		/* cached ctrl pins: BL/RS/RW/E zeros except BL maybe */
	struct k_mutex		lock;
};

struct hd44780_pcf8574_api
{
	int (*write)(const struct device *dev, const char *s, size_t n);
	int (*clear)(const struct device *dev);
	int (*home)(const struct device *dev);
	int (*set_cursor)(const struct device *dev, uint8_t col, uint8_t row);
	int (*control)(const struct device *dev, bool display, bool cursor, bool blink);
};

#endif

