/**
 * @file	hd44780_pcf8574.c
 * @author	Utsav Joshi
 * @version	1.0
 * @date	2025-09-22
 *
 * @brief	Zephyr out-of-tree driver for HD44780 LCD via PCF8574 I2C backpack (4-bit mode).
 *
 * @details
 *	- PCF8574 bit map (default):
 *		- P0 RS, P1 RW, P2 E, P3 BL, P4..P7 D4..D7
 *	- Backlight polarity is selectable by DeviceTree boolean property @c bl-active-low.
 *	- Minimal public API exposed via @c include/hd44780_pcf8574.h .
 *
 * @par Design notes
 *	- From-scratch driver (no auxdisplay reuse). Uses Zephyr device model + DT glue.
 *	- Timing margins follow datasheet (safe delays around strobe and clear/home).
 *	- I2C access via @c i2c_dt_spec ; all transactions are synchronous.
 *
 * @par Thread-safety
 *	- Public API functions MAY be called from multiple contexts; a mutex protects internal
 *	  @c ctrl byte composition and bus writes where needed.
 *
 * @par Version History
 *	| Ver | Date       | Notes                    |
 *	|-----|------------|--------------------------|
 *	| 1.0 | 2025-09-22 | Initial public version   |
 */

#include "hd44780_pcf8574.h"
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>

LOG_MODULE_REGISTER(hd44780_pcf8574, LOG_LEVEL_INF);

/* ---------- Driver compatible (MUST match binding) ---------- */
#define DT_DRV_COMPAT hit_hd44780_pcf8574

/* ---------- HD44780 command mnemonics ---------- */
/** @name Core command opcodes
 *  @{
 */
#define CMD_CLEAR	0x01	/**< Clear display, set DDRAM addr=0 */
#define CMD_HOME	0x02	/**< Return home (DDRAM=0), cursor home */
#define CMD_ENTRY	0x04	/**< Entry mode set base opcode (I/D,S) */
#define CMD_DISPLAY	0x08	/**< Display control base opcode (D,C,B) */
#define CMD_SHIFT	0x10	/**< Cursor/Display shift base opcode */
#define CMD_FUNC	0x20	/**< Function set base opcode (DL,N,F) */
#define CMD_CGRAM	0x40	/**< Set CGRAM address base opcode */
#define CMD_DDRAM	0x80	/**< Set DDRAM address base opcode */
/** @} */

/** @name Bit fields for ENTRY, DISPLAY, FUNC
 *  @{
 */
#define ENTRY_ID	BIT(1)	/**< Increment cursor (I/D) */
#define DISPLAY_D	BIT(2)	/**< Display ON */
#define DISPLAY_C	BIT(1)	/**< Cursor ON */
#define DISPLAY_B	BIT(0)	/**< Blink ON */

#define FUNC_DL		BIT(4)	/**< Data length 1=8-bit (use 0 for 4-bit) */
#define FUNC_N		BIT(3)	/**< Number of lines 1=2-line */
#define FUNC_F		BIT(2)	/**< Font 1=5x10 (use 0 for 5x8) */
/** @} */

/* ---------- Row mapping helpers ---------- */

/**
 * @brief	Row -> DDRAM base mapping entry.
 */
struct hd44780_pcf8574_row_base
{
	uint8_t row;	/**< Zero-based row number. */
	uint8_t base;	/**< Base DDRAM address for row. */
};

/**
 * @brief	Compute DDRAM address for (row,col).
 *
 * @param col	Zero-based column index.
 * @param row	Zero-based row index.
 * @param rows	Total rows on panel (2 or 4 typical).
 *
 * @return	7-bit DDRAM address to OR with @c CMD_DDRAM .
 *
 * @note	16x2: bases 0x00,0x40; 20x4: 0x00,0x40,0x14,0x54.
 */
static inline uint8_t ddram_addr(uint8_t col, uint8_t row, uint8_t rows)
{
	static const struct hd44780_pcf8574_row_base map2[] = {
		{0, 0x00}, {1, 0x40},
	};
	static const struct hd44780_pcf8574_row_base map4[] = {
		{0, 0x00}, {1, 0x40}, {2, 0x14}, {3, 0x54},
	};
	const struct hd44780_pcf8574_row_base *m = (rows > 2) ? map4 : map2;
	return m[row].base + col;
}

/* ---------- Low-level I2C helpers ---------- */

/**
 * @brief	Write one byte to PCF8574 (no E pulse).
 *
 * @param dev	Device instance.
 * @param v	PCF8574 output byte (RS/RW/E/BL/D4..D7).
 * @retval 0	On success.
 * @retval <0	Negative errno from @c i2c_write_dt .
 *
 * @note	This simply latches the new output state; use @c strobe() to pulse E.
 */
static int pcf_write(const struct device *dev, uint8_t v)
{
	const struct hd44780_pcf8574_cfg *cfg = dev->config;
	return i2c_write_dt(&cfg->i2c, &v, 1);
}

/**
 * @brief	Generate E strobe to latch the current nibble.
 *
 * @param dev	Device instance.
 * @param v	Current control byte (RS/RW/BL + D4..D7 already set).
 * @retval 0	On success.
 * @retval <0	If any underlying I2C write fails.
 *
 * @note	Timing margins:
 *	- E high width: >= 450 ns (we busy-wait ~1 µs).
 *	- Command cycle: >= 37 µs (we wait ~50 µs).
 */
static int strobe(const struct device *dev, uint8_t v)
{
	int r = 0;
	r |= pcf_write(dev, v | P_E);
	k_busy_wait(1);		/* >= 450 ns */
	r |= pcf_write(dev, v & ~P_E);
	k_busy_wait(50);	/* >= 37 us (safe) */
	return r;
}

/**
 * @brief	Write a 4-bit nibble via PCF8574 and latch with E pulse.
 *
 * @param dev	Device instance.
 * @param nibble	Low 4 bits are mapped to D4..D7.
 * @param rs	@c true for data, @c false for command.
 * @retval 0	On success.
 * @retval <0	From @c strobe / I2C layer.
 */
static int write4(const struct device *dev, uint8_t nibble, bool rs)
{
	struct hd44780_pcf8574_data *data = dev->data;
	uint8_t v = data->ctrl;

	if (rs) v |= P_RS;
	else v &= ~P_RS;

	if (nibble & 0x01) v |= P_D4; else v &= ~P_D4;
	if (nibble & 0x02) v |= P_D5; else v &= ~P_D5;
	if (nibble & 0x04) v |= P_D6; else v &= ~P_D6;
	if (nibble & 0x08) v |= P_D7; else v &= ~P_D7;

	return strobe(dev, v);
}

/**
 * @brief	Send full 8-bit value (two nibbles).
 *
 * @param dev	Device instance.
 * @param byte	8-bit command or data byte.
 * @param rs	@c true for data, @c false for command.
 * @retval 0	On success.
 * @retval <0	On first failure.
 */
static int send(const struct device *dev, uint8_t byte, bool rs)
{
	int r = 0;
	r |= write4(dev, (byte >> 4) & 0x0F, rs);
	r |= write4(dev, (byte >> 0) & 0x0F, rs);
	return r;
}

/**
 * @brief	Send a command byte.
 * @param dev	Device instance.
 * @param c	Command value.
 * @return	0 on success, negative errno on error.
 */
static int cmd(const struct device *dev, uint8_t c)
{
	return send(dev, c, false);
}

/**
 * @brief	Send a data (character) byte.
 * @param dev	Device instance.
 * @param ch	Character.
 * @return	0 on success, negative errno on error.
 */
static int data_write(const struct device *dev, uint8_t ch)
{
	return send(dev, ch, true);
}

/* ---------- Backlight ---------- */

/**
 * @brief	Apply logical backlight state to PCF8574 and push without strobe.
 *
 * @param dev	Device instance.
 * @param on	Logical ON/OFF requested by app.
 * @note	Polarity is XORed with @c bl_active_low from DT.
 */
static void backlight_apply(const struct device *dev, bool on)
{
	const struct hd44780_pcf8574_cfg *cfg = dev->config;
	struct hd44780_pcf8574_data *data = dev->data;
	bool phys_on = on ^ cfg->bl_active_low;

	if (phys_on) {
		data->ctrl |= P_BL;
	} else {
		data->ctrl &= ~P_BL;
	}
	(void)pcf_write(dev, data->ctrl);
}

/* ---------- Public API (function pointers) ---------- */

/**
 * @brief	Clear display and wait for completion.
 * @param dev	Device instance.
 * @return	0 on success, negative errno on error.
 */
static int fn_clear(const struct device *dev)
{
        struct hd44780_pcf8574_data *data = dev->data;
	k_mutex_lock(&data->lock,K_FOREVER);
	int r = cmd(dev, CMD_CLEAR);
	k_msleep(2);	/* ~1.52 ms typ */
	k_mutex_unlock(&data->lock);
	return r;
}

/**
 * @brief	Return cursor and DDRAM address to 0.
 * @param dev	Device instance.
 * @return	0 on success, negative errno on error.
 */
static int fn_home(const struct device *dev)
{

        struct hd44780_pcf8574_data *data = dev->data;
	k_mutex_lock(&data->lock,K_FOREVER);
	int r = cmd(dev, CMD_HOME);
	k_msleep(2);
	k_mutex_unlock(&data->lock);
	return r;
}

/**
 * @brief	Set cursor to (row,col).
 *
 * @param dev	Device instance.
 * @param col	Zero-based column.
 * @param row	Zero-based row.
 * @retval 0	On success.
 * @retval -EINVAL	If coordinates exceed geometry.
 */
static int fn_set_cursor(const struct device *dev, uint8_t col, uint8_t row)
{
	const struct hd44780_pcf8574_cfg *cfg = dev->config;
        struct hd44780_pcf8574_data *data = dev->data;
	int r;
	if (row >= cfg->rows) return -EINVAL;
	if (col >= cfg->cols) return -EINVAL;

	uint8_t addr = ddram_addr(col, row, cfg->rows);
	k_mutex_lock(&data->lock,K_FOREVER);
	r= cmd(dev, CMD_DDRAM | (addr & 0x7F));
	k_mutex_unlock(&data->lock);
	return r;
}

/**
 * @brief	Set display/cursor/blink flags.
 *
 * @param dev	Device instance.
 * @param display	Display ON if true.
 * @param cursor	Cursor ON if true.
 * @param blink		Blink ON if true.
 * @return	0 on success, negative errno on error.
 */
static int fn_control(const struct device *dev, bool display, bool cursor, bool blink)
{
        struct hd44780_pcf8574_data *data = dev->data;
	int r;
	uint8_t v = CMD_DISPLAY;
	if (display) v |= DISPLAY_D;
	if (cursor)  v |= DISPLAY_C;
	if (blink)   v |= DISPLAY_B;
	k_mutex_lock(&data->lock,K_FOREVER);
	r=cmd(dev, v);
	k_mutex_unlock(&data->lock);
	return r;
}

/**
 * @brief	Write @p n bytes from @p s at current cursor.
 *
 * @param dev	Device instance.
 * @param s	Pointer to buffer.
 * @param n	Number of bytes to write.
 * @retval 0	On success.
 * @retval <0	On first error; error is logged.
 */
static int fn_write(const struct device *dev, const char *s, size_t n)
{
        struct hd44780_pcf8574_data *data = dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);
	for (size_t i = 0; i < n; ++i) {
		int r = data_write(dev, (uint8_t)s[i]);
		if (r) {
			LOG_ERR("write error %d", r);
			return r;
		}
	}
	k_mutex_unlock(&data->lock);           
	return 0;
}

/** @brief Public API vtable. */
static const struct hd44780_pcf8574_api api = {
	.write = fn_write,		/**< Write buffer. */
	.clear = fn_clear,		/**< Clear display. */
	.home = fn_home,		/**< Return home. */
	.set_cursor = fn_set_cursor,	/**< Set cursor position. */
	.control = fn_control,		/**< Display/cursor/blink control. */
};

/* ---------- Device init ---------- */

/**
 * @brief	Device init: I2C ready check, backlight, force 4-bit, defaults.
 *
 * @param dev	Device instance.
 * @retval 0	On success.
 * @retval -ENODEV	If I2C bus is not ready.
 * @retval <0	For early I2C failures during sequencing.
 *
 * @note	Sequencing (datasheet):
 *	- Wait >40 ms after VCC ≥ 2.7V.
 *	- Send 0x3 (high nibble) three times, then 0x2 to enter 4-bit.
 *	- Function set (N depends on rows, F=0).
 *	- Display off, clear, entry mode (increment, no shift), display on.
 */
static int hd44780_init(const struct device *dev)
{
	const struct hd44780_pcf8574_cfg *cfg = dev->config;
	struct hd44780_pcf8574_data *data = dev->data;

	if (!device_is_ready(cfg->i2c.bus)) {
		return -ENODEV;
	}

	k_mutex_init(&data->lock);

	LOG_INF("init: dev=%s bus=%s addr=0x%02x cols=%u rows=%u bl_active_low=%u",
		dev->name, cfg->i2c.bus->name, cfg->i2c.addr,
		cfg->cols, cfg->rows, cfg->bl_active_low);

	/* Start from a known control state (RW=0, RS=0, E=0, BL per Kconfig/DT) */
	data->ctrl = 0x00;
	data->ctrl &= ~(P_RW | P_RS | P_E);
	backlight_apply(dev, IS_ENABLED(CONFIG_HD44780_PCF8574_BACKLIGHT_ON_BOOT));

	/* >40 ms after VCC rises to 2.7 V */
	k_msleep(50);

	/* Force 4-bit interface: write high nibble 0x3 thrice, then 0x2 */
	if (write4(dev, 0x03, false) < 0) return -EIO;
	k_msleep(5);
	if (write4(dev, 0x03, false) < 0) return -EIO;
	k_msleep(5);
	if (write4(dev, 0x03, false) < 0) return -EIO;
	k_msleep(1);
	if (write4(dev, 0x02, false) < 0) return -EIO;
	k_msleep(1);

	/* Function set: DL=0 (4-bit), N depends on rows, F=0 (5x8) */
	uint8_t func = CMD_FUNC | (cfg->rows > 1 ? FUNC_N : 0);
	if (cmd(dev, func) < 0) return -EIO;
	k_msleep(1);

	/* Display off */
	if (cmd(dev, CMD_DISPLAY) < 0) return -EIO;
	k_msleep(1);

	/* Clear */
	if (fn_clear(dev) < 0) return -EIO;

	/* Entry mode: increment, no shift */
	if (cmd(dev, CMD_ENTRY | ENTRY_ID) < 0) return -EIO;
	k_msleep(1);

	/* Display ON, cursor/blink off */
	if (fn_control(dev, true, false, false) < 0) return -EIO;

	LOG_INF("init: setup done");
	return 0;
}

/* ---------- DT glue & instance generation ---------- */

/**
 * @brief	Per-instance constant configuration.
 *
 * @details
 *	- @c i2c : resolved I2C DT spec (bus + addr).
 *	- @c cols / @c rows : geometry (fallbacks from Kconfig if absent).
 *	- @c bl_active_low : backlight polarity flag from DT (default false).
 */

/* Generate cfg struct per DT instance */
#define HD44780_CFG(inst) \
	static const struct hd44780_pcf8574_cfg cfg_##inst = { \
		.i2c = I2C_DT_SPEC_INST_GET(inst), \
		.cols = DT_INST_PROP_OR(inst, columns, CONFIG_HD44780_PCF8574_DEFAULT_COLS), \
		.rows = DT_INST_PROP_OR(inst, rows,    CONFIG_HD44780_PCF8574_DEFAULT_ROWS), \
		.bl_active_low = DT_INST_PROP_OR(inst, bl_active_low, false), \
	};

/* Generate data struct per DT instance */
#define HD44780_DATA(inst) \
	static struct hd44780_pcf8574_data data_##inst;

/* Generate device per DT instance */
#define HD44780_DEV(inst) \
	DEVICE_DT_INST_DEFINE(inst, \
		hd44780_init, NULL, \
		&data_##inst, &cfg_##inst, \
		POST_KERNEL, CONFIG_HD44780_PCF8574_INIT_PRIORITY, \
		&api);

/* Expand for all status = "okay" instances */
DT_INST_FOREACH_STATUS_OKAY(HD44780_CFG)
DT_INST_FOREACH_STATUS_OKAY(HD44780_DATA)
DT_INST_FOREACH_STATUS_OKAY(HD44780_DEV)

