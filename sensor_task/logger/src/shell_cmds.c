#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/logging/log.h>
#include <ctype.h>
#include <stdlib.h>

#include "fs_log.h"
#include "shell_cmds.h"

LOG_MODULE_REGISTER(sens_sh, LOG_LEVEL_INF);
static struct k_mutex g_rate_lock;

extern volatile float g_last_temp_c, g_last_hum, g_last_press_hpa;
extern volatile float g_last_ax, g_last_ay, g_last_az;

uint32_t g_period_ms = 1000;


static int cmd_sens_show(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc); ARG_UNUSED(argv);
	shell_print(sh, "T=%.2f C, H=%.1f %%, P=%.2f hPa, A=[%.3f,%.3f,%.3f] g",
		(double)g_last_temp_c, (double)g_last_hum, (double)g_last_press_hpa,
		(double)g_last_ax, (double)g_last_ay, (double)g_last_az);
	return 0;
}

static int cmd_sens_cat(const struct shell *sh, size_t argc, char **argv)
{
	size_t maxb = SIZE_MAX;
	if (argc == 2) {
		maxb = (size_t)strtoul(argv[1], NULL, 10);
	}
	(void)fslog_cat(maxb);
	return 0;
}

static int cmd_sens_clear(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc); ARG_UNUSED(argv);
	int rc = fslog_clear();
	shell_print(sh, rc ? "clear failed: %d" : "cleared", rc);
	return rc;
}

static int cmd_sens_rate(const struct shell *sh, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_print(sh, "rate: %u ms", sens_get_period_ms());
		return 0;
	}
	uint32_t ms = (uint32_t)strtoul(argv[1], NULL, 10);
	if (ms < 100) ms = 100;
	sens_update_period_ms(ms);
	shell_print(sh, "rate set: %u ms", ms);
	return 0;
}

static int cmd_sens_live(const struct shell *sh, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_print(sh, "usage: sens live on|off");
		return 0;
	}
	bool en = (argv[1][0] == 'o' && argv[1][1] == 'n');
	sens_set_live(en);
	shell_print(sh, "live: %s", en ? "on" : "off");
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_sens,
	SHELL_CMD(show, NULL, "show last sample", cmd_sens_show),
	SHELL_CMD(cat,  NULL, "print log (opt: <max_bytes>)", cmd_sens_cat),
	SHELL_CMD(clear,NULL, "truncate log", cmd_sens_clear),
	SHELL_CMD(rate, NULL, "get/set period ms", cmd_sens_rate),
	SHELL_CMD(live, NULL, "enable/disable live prints", cmd_sens_live),
	SHELL_SUBCMD_SET_END
);
void sens_update_period_ms(uint32_t ms)
{
	k_mutex_init(&g_rate_lock);
        k_mutex_lock(&g_rate_lock, K_FOREVER);
        g_period_ms = ms;
        k_mutex_unlock(&g_rate_lock);
}

uint32_t sens_get_period_ms(void)
{
        uint32_t ms;
	k_mutex_init(&g_rate_lock);
        k_mutex_lock(&g_rate_lock, K_FOREVER);
        ms = g_period_ms;
        k_mutex_unlock(&g_rate_lock);
        return ms;
}

SHELL_CMD_REGISTER(sens, &sub_sens, "sensor logging controls", NULL);

void sens_shell_register(void) { /* linker pulls via above */ }

