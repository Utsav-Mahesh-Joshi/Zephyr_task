/* shell_threads.c : 1-minute tickless sampler
 *	- Workers only update shared sensor_data (no FS writes)
 *	- Coordinator triggers HT -> PRESS -> IMU, snapshots data, writes once
 *	- No device runtime PM; CPU/system sleeps while idle
 */

#include "shell_threads.h"
#include <zephyr/kernel.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <string.h>

/* ------------ config ------------ */
#define LOG_PERIOD_MS		6000		/* 6 s (set 60000 for 60 s) */
#define SENSOR_PATH		"/lfs/sensor.txt"

LOG_MODULE_REGISTER(shell_threads);

/* ------------ threads & stacks ------------ */
static struct k_thread		hum_thread_data, press_thread_data, imu_thread_data, coord_thread_data;
static K_THREAD_STACK_DEFINE(hum_stack,   2048);
static K_THREAD_STACK_DEFINE(press_stack, 2048);
static K_THREAD_STACK_DEFINE(imu_stack,   2048);
static K_THREAD_STACK_DEFINE(coord_stack, 3072);

static k_tid_t			hum_tid, press_tid, imu_tid, coord_tid;

/* ------------ semaphores (chain + control) ------------ */
K_SEM_DEFINE(semHT,	0, 1);		/* coordinator -> HT start */
K_SEM_DEFINE(semPress,	0, 1);		/* HT -> PRESS */
K_SEM_DEFINE(semGyro,	0, 1);		/* PRESS -> IMU */
K_SEM_DEFINE(semDone,	0, 1);		/* IMU -> coordinator (cycle done) */

/* ------------ shared data ------------ */
struct sensor_data {
	float		temp;
	float		hum;
	float		press;
	float		ax, ay, az;
	float		gx, gy, gz;
	bool		ht_ok;
	bool		press_ok;
	bool		imu_ok;
};

static struct sensor_data	g_sd;
static struct k_mutex		g_sd_mtx;

/* ------------ helpers ------------ */
static inline void ts_now(uint32_t *sec, uint32_t *mms)
{
	uint64_t ms = k_uptime_get();
	*sec = (uint32_t)(ms / 1000U);
	*mms = (uint32_t)(ms % 1000U);
}

/* parse helpers: your existing *_get_string() return formatted text.
 * Below we parse the few numbers we need. If you control those functions,
 * consider adding dedicated getters to avoid sscanf().
 */
static bool parse_ht(const char *s, float *t, float *h)
{
	/* expects: "Temperature: %.1f C, Humidity: %.1f %%\n" */
	return (sscanf(s, "Temperature: %f C, Humidity: %f %%", t, h) == 2);
}
static bool parse_press(const char *s, float *p)
{
	/* expects: "Pressure: %.1f kPa\n" */
	return (sscanf(s, "Pressure: %f", p) == 1);
}
static bool parse_imu(const char *s, float *ax, float *ay, float *az, float *gx, float *gy, float *gz)
{
	/* expects: "Accel: ax, ay, az | Gyro: gx, gy, gz\n" */
	return (sscanf(s, "Accel: %f, %f, %f | Gyro: %f, %f, %f", ax, ay, az, gx, gy, gz) == 6);
}

/* ------------ worker threads (no FS; update g_sd only) ------------ */
void hum_thread(void *a, void *b, void *c)
{
	char	buf[128];

	for (;;)
	{
		k_sem_take(&semHT, K_FOREVER);

		bool	ok = false;
		if (hum_temp_sensor_get_string(buf, sizeof(buf)) > 0) {
			float	t, h;
			ok = parse_ht(buf, &t, &h);
			if (ok) {
				k_mutex_lock(&g_sd_mtx, K_FOREVER);
				g_sd.temp = t;
				g_sd.hum  = h;
				g_sd.ht_ok = true;
				k_mutex_unlock(&g_sd_mtx);
			}
		}

		if (!ok) {
			k_mutex_lock(&g_sd_mtx, K_FOREVER);
			g_sd.ht_ok = false;
			k_mutex_unlock(&g_sd_mtx);
		}

		k_sem_give(&semPress);
	}
}

void press_thread(void *a, void *b, void *c)
{
	char	buf[128];

	for (;;)
	{
		k_sem_take(&semPress, K_FOREVER);

		bool	ok = false;
		if (pressure_sensor_get_string(buf, sizeof(buf)) > 0) {
			float	p;
			ok = parse_press(buf, &p);
			if (ok) {
				k_mutex_lock(&g_sd_mtx, K_FOREVER);
				g_sd.press = p;
				g_sd.press_ok = true;
				k_mutex_unlock(&g_sd_mtx);
			}
		}

		if (!ok) {
			k_mutex_lock(&g_sd_mtx, K_FOREVER);
			g_sd.press_ok = false;
			k_mutex_unlock(&g_sd_mtx);
		}

		k_sem_give(&semGyro);
	}
}

void imu_thread(void *a, void *b, void *c)
{
	char	buf[192];

	for (;;)
	{
		k_sem_take(&semGyro, K_FOREVER);

		bool	ok = false;
		if (imu_sensor_get_string(buf, sizeof(buf)) > 0) {
			float	ax, ay, az, gx, gy, gz;
			ok = parse_imu(buf, &ax, &ay, &az, &gx, &gy, &gz);
			if (ok) {
				k_mutex_lock(&g_sd_mtx, K_FOREVER);
				g_sd.ax = ax; g_sd.ay = ay; g_sd.az = az;
				g_sd.gx = gx; g_sd.gy = gy; g_sd.gz = gz;
				g_sd.imu_ok = true;
				k_mutex_unlock(&g_sd_mtx);
			}
		}

		if (!ok) {
			k_mutex_lock(&g_sd_mtx, K_FOREVER);
			g_sd.imu_ok = false;
			k_mutex_unlock(&g_sd_mtx);
		}

		k_sem_give(&semDone);
	}
}

/* ------------ coordinator (periodic, writes once) ------------ */
static void coordinator_thread(void *a, void *b, void *c)
{
	int64_t		next_deadline = k_uptime_get();
	struct fs_file_t	file;
	char		line[320];

	for (;;)
	{
		next_deadline += LOG_PERIOD_MS;

		/* start chain and wait for completion */
		k_sem_give(&semHT);
		k_sem_take(&semDone, K_FOREVER);

		/* snapshot shared data under mutex */
		struct sensor_data snap;
		k_mutex_lock(&g_sd_mtx, K_FOREVER);
		snap = g_sd;
		k_mutex_unlock(&g_sd_mtx);

		/* timestamp and single-file-write */
		uint32_t sec, mms; ts_now(&sec, &mms);

		/* format one compact line (adapt as you prefer) */
		int n = snprintk(line, sizeof(line),
			"[%lu.%03lu] HT[%c] T=%.2fC H=%.2f%% | P[%c]=%.2fkPa | "
			"IMU[%c] A=(%.2f,%.2f,%.2f) G=(%.2f,%.2f,%.2f)\r\n",
			sec, mms,
			snap.ht_ok ? 'Y' : 'N',  snap.temp, snap.hum,
			snap.press_ok ? 'Y' : 'N', snap.press,
			snap.imu_ok ? 'Y' : 'N', snap.ax, snap.ay, snap.az, snap.gx, snap.gy, snap.gz);

		if (n > 0) {
			fs_file_t_init(&file);
			if (fs_open(&file, SENSOR_PATH, FS_O_CREATE | FS_O_WRITE | FS_O_APPEND) == 0) {
				fs_write(&file, line, (size_t)n);
				fs_close(&file);
			}
		}

		/* sleep until next absolute boundary (tickless → one-shot) */
		int64_t	now = k_uptime_get();
		int64_t	sleep_ms = next_deadline - now;
		if (sleep_ms < 1) sleep_ms = 1;
		k_msleep((int)sleep_ms);
	}
}

/* ------------ shell cmds ------------ */
static int cmd_start_sensor(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc); ARG_UNUSED(argv);

	if (!hum_tid) {
		k_mutex_init(&g_sd_mtx);
		memset(&g_sd, 0, sizeof(g_sd));

		hum_tid = k_thread_create(&hum_thread_data, hum_stack,
			K_THREAD_STACK_SIZEOF(hum_stack),
			hum_thread, NULL, NULL, NULL, 5, 0, K_NO_WAIT);
		shell_print(sh, "HT worker started.");
	}
	if (!press_tid) {
		press_tid = k_thread_create(&press_thread_data, press_stack,
			K_THREAD_STACK_SIZEOF(press_stack),
			press_thread, NULL, NULL, NULL, 5, 0, K_NO_WAIT);
		shell_print(sh, "PRESS worker started.");
	}
	if (!imu_tid) {
		imu_tid = k_thread_create(&imu_thread_data, imu_stack,
			K_THREAD_STACK_SIZEOF(imu_stack),
			imu_thread, NULL, NULL, NULL, 5, 0, K_NO_WAIT);
		shell_print(sh, "IMU worker started.");
	}
	if (!coord_tid) {
		coord_tid = k_thread_create(&coord_thread_data, coord_stack,
			K_THREAD_STACK_SIZEOF(coord_stack),
			coordinator_thread, NULL, NULL, NULL, 4, 0, K_NO_WAIT);
		shell_print(sh, "Coordinator started (period=%d ms).", LOG_PERIOD_MS);
	}

	return 0;
}

static int cmd_stop_sensors(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc); ARG_UNUSED(argv);

	if (coord_tid) { k_thread_abort(coord_tid); coord_tid = NULL; shell_print(sh, "Coordinator stopped."); }
	if (hum_tid)   { k_thread_abort(hum_tid);   hum_tid   = NULL; shell_print(sh, "HT worker stopped."); }
	if (press_tid) { k_thread_abort(press_tid); press_tid = NULL; shell_print(sh, "PRESS worker stopped."); }
	if (imu_tid)   { k_thread_abort(imu_tid);   imu_tid   = NULL; shell_print(sh, "IMU worker stopped."); }

	/* drain sems for a clean next start */
	while (k_sem_count_get(&semHT)    > 0) k_sem_take(&semHT,    K_NO_WAIT);
	while (k_sem_count_get(&semPress) > 0) k_sem_take(&semPress, K_NO_WAIT);
	while (k_sem_count_get(&semGyro)  > 0) k_sem_take(&semGyro,  K_NO_WAIT);
	while (k_sem_count_get(&semDone)  > 0) k_sem_take(&semDone,  K_NO_WAIT);

	return 0;
}

static int cmd_clear_logs(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc); ARG_UNUSED(argv);

	int ret = fs_unlink(SENSOR_PATH);
	if (ret < 0 && ret != -ENOENT) {
		shell_fprintf(shell, SHELL_ERROR, "Failed to remove %s (%d)\n", SENSOR_PATH, ret);
		return ret;
	}
	shell_fprintf(shell, SHELL_NORMAL, "Log cleared: %s\n", SENSOR_PATH);
	return 0;
}

/* ------------ shell reg ------------ */
SHELL_STATIC_SUBCMD_SET_CREATE(sub_sensors,
	SHELL_CMD(start_sensors,	NULL, "Start periodic sensor logging",	cmd_start_sensor),
	SHELL_CMD(stop_sensors,		NULL, "Stop sensor logging",		cmd_stop_sensors),
	SHELL_CMD(clear_logs,		NULL, "Clear sensor log file",		cmd_clear_logs),
	SHELL_SUBCMD_SET_END
);
SHELL_CMD_REGISTER(sensors, &sub_sensors, "Sensor logging commands", NULL);

