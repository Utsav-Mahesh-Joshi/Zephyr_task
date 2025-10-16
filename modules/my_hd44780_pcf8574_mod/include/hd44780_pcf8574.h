#ifndef HD44780_PCF8574_H
#define HD44780_PCF8574_H

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>		/* for strlen in hd44780_print() */

/*
 * Public driver API vtable (shape must match the one used in the driver .c)
 * Exposing it here lets the inline wrappers cast dev->api safely.
 */
struct hd44780_pcf8574_api
{
	int (*write)(const struct device *dev, const char *s, size_t n);
	int (*clear)(const struct device *dev);
	int (*home)(const struct device *dev);
	int (*set_cursor)(const struct device *dev, uint8_t col, uint8_t row);
	int (*control)(const struct device *dev, bool display, bool cursor, bool blink);
};

#define HD44780_API(dev) \
	((const struct hd44780_pcf8574_api *)((dev)->api))


static inline int hd44780_write(const struct device *dev, const char *s, size_t n)
{
	return HD44780_API(dev)->write(dev, s, n);
}

static inline int hd44780_print(const struct device *dev, const char *s)
{
	size_t n = (s != NULL) ? strlen(s) : 0U;
	return HD44780_API(dev)->write(dev, s, n);
}

static inline int hd44780_clear(const struct device *dev)
{
	return HD44780_API(dev)->clear(dev);
}

static inline int hd44780_home(const struct device *dev)
{
	return HD44780_API(dev)->home(dev);
}

static inline int hd44780_set_cursor(const struct device *dev, uint8_t col, uint8_t row)
{
	return HD44780_API(dev)->set_cursor(dev, col, row);
}

static inline int hd44780_control(const struct device *dev, bool display, bool cursor, bool blink)
{
	return HD44780_API(dev)->control(dev, display, cursor, blink);
}

#endif	/* HD44780_PCF8574_H */

