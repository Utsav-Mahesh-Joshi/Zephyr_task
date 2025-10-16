/* shell_threads.c : independent periodic workers → msgq → 20s batched file writes
 *	- Each worker samples on its own interval and enqueues a preformatted line
 *	- Logger thread buffers queue items in RAM and appends them to LittleFS every 20 s
 */

#include "shell_threads.h"
#include <zephyr/kernel.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <string.h>
#include <stdbool.h>

LOG_MODULE_REGISTER(shell_threads);

/* ------------ config ------------ */
#define SENSOR_PATH			"/lfs/sensor.txt"

/* per-sensor sampling periods (tune as you like) */
#define HT_PERIOD_MS			2000		/* 2 s */
#define PRESS_PERIOD_MS			5000		/* 5 s */
#define IMU_PERIOD_MS			1000		/* 1 s */

/* batched file write period */
#define FLUSH_PERIOD_MS			20000		/* 20 s */

static k_tid_t                  hum_tid, press_tid, imu_tid, log_tid;
static struct k_thread          hum_thread_data, press_thread_data, imu_thread_data, log_thread_data;

static K_THREAD_STACK_DEFINE(hum_stack,   2048);
static K_THREAD_STACK_DEFINE(press_stack, 2048);
static K_THREAD_STACK_DEFINE(imu_stack,   2048);
static K_THREAD_STACK_DEFINE(log_stack,  2048);


/* ------------ semaphores (chain + control) ------------ */
K_SEM_DEFINE(semHT,     1, 1);          /* IMU -> HT */
K_SEM_DEFINE(semPress,  0, 1);          /* HT -> PRESS */
K_SEM_DEFINE(semGyro,   0, 1);          /* PRESS -> IMU */


/* ------------ shared data ------------ */
struct sensor_data {
        float           temp;
        float           hum;
        float           press;
        float           ax, ay, az;
        float           gx, gy, gz;
        bool            ht_ok;
        bool            press_ok;
        bool            imu_ok;
};
#define Q_DEPTH 30
K_MSGQ_DEFINE(msgq, 128, Q_DEPTH, 1);

static struct sensor_data       g_sd;
static struct k_mutex           g_sd_mtx;
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
static inline void ts_now(uint32_t *sec, uint32_t *mms)
{
        uint64_t ms = k_uptime_get();
        *sec = (uint32_t)(ms / 1000U);
        *mms = (uint32_t)(ms % 1000U);
}

void hum_thread(void *a, void *b, void *c)
{
        char    buf[128];
	char 	out[128];
	uint32_t mms,sec;
		
        for (;;)
        {
                k_sem_take(&semHT, K_FOREVER);
		
		ts_now(&sec,&mms);
                bool    ok = false;
                if (hum_temp_sensor_get_string(buf, sizeof(buf)) > 0) {
                        float   t, h;
                        ok = parse_ht(buf, &t, &h);
			snprintf(out,sizeof(out),"[%lu.%03lu]:%s",sec,mms,buf);
                        if (ok) {
                                k_mutex_lock(&g_sd_mtx, K_FOREVER);
                                g_sd.temp = t;
                                g_sd.hum  = h;
                                g_sd.ht_ok = true;
				k_msgq_put(&msgq,out,K_FOREVER);
                                k_mutex_unlock(&g_sd_mtx);
                        }
                }
               if (!ok) {
                        k_mutex_lock(&g_sd_mtx, K_FOREVER);
                        g_sd.ht_ok = false;
			LOG_ERR("HT read failed!");
                        k_mutex_unlock(&g_sd_mtx);
                }

                k_sem_give(&semPress);
		k_msleep(HT_PERIOD_MS);
        }
}

void press_thread(void *a, void *b, void *c)
{
        char    buf[128];
	char 	out[128];

	uint32_t mms,sec;
        for (;;)
        {
                k_sem_take(&semPress, K_FOREVER);

		ts_now(&sec,&mms);
                bool    ok = false;
                if (pressure_sensor_get_string(buf, sizeof(buf)) > 0) {
                        float   p;
                        ok = parse_press(buf, &p);
			snprintf(out,sizeof(out),"[%lu.%03lu]:%s",sec,mms,buf);
                        if (ok) {
                                k_mutex_lock(&g_sd_mtx, K_FOREVER);
                                g_sd.press = p;
                                g_sd.press_ok = true;
				k_msgq_put(&msgq,out,K_FOREVER);
                                k_mutex_unlock(&g_sd_mtx);
                        }
                }
	        if (!ok) {
                k_mutex_lock(&g_sd_mtx, K_FOREVER);
                g_sd.press_ok = false;
		LOG_ERR("Press read failed!");
                k_mutex_unlock(&g_sd_mtx);
                }

                k_sem_give(&semGyro);
		k_msleep(PRESS_PERIOD_MS);
        }
}
void imu_thread(void *a, void *b, void *c)
{
        char    buf[128];
	char 	out[128];
	
	uint32_t mms,sec;

        for (;;)
        {
                k_sem_take(&semGyro, K_FOREVER);

		ts_now(&sec,&mms);
                bool    ok = false;
                if (imu_sensor_get_string(buf, sizeof(buf)) > 0) {
                        float   ax, ay, az, gx, gy, gz;
                        ok = parse_imu(buf, &ax, &ay, &az, &gx, &gy, &gz);
			snprintf(out,sizeof(out),"[%lu.%03lu]:%s",sec,mms,buf);
                        if (ok) {
                                k_mutex_lock(&g_sd_mtx, K_FOREVER);
                                g_sd.ax = ax; g_sd.ay = ay; g_sd.az = az;
                                g_sd.gx = gx; g_sd.gy = gy; g_sd.gz = gz;
                                g_sd.imu_ok = true;
				k_msgq_put(&msgq,out,K_FOREVER);
                                k_mutex_unlock(&g_sd_mtx);
                        }
                }

                if (!ok) {
                        k_mutex_lock(&g_sd_mtx, K_FOREVER);
                        g_sd.imu_ok = false;
			LOG_ERR("Imu read failed!");
                        k_mutex_unlock(&g_sd_mtx);
                }
                k_sem_give(&semHT);
		k_msleep(IMU_PERIOD_MS);
        }
}
void log_thread(void *a, void *b, void *c)
{
	struct fs_file_t        file;
	char 			line[128];
	size_t 			n;
	uint32_t 		sec, mms; 
	uint32_t		message_num;	
	ts_now(&sec, &mms);

	for(;;)
	{
		k_msleep(FLUSH_PERIOD_MS);
		k_mutex_lock(&g_sd_mtx,K_FOREVER);

		while((message_num=k_msgq_num_used_get(&msgq))>0)
		{
			fs_file_t_init(&file);
			k_msgq_get(&msgq,&line,K_FOREVER);
			n=strlen(line)+1;
			if (fs_open(&file, SENSOR_PATH, FS_O_CREATE | FS_O_WRITE | FS_O_APPEND) == 0)
			{
				fs_write(&file, line, (size_t)n);
				fs_close(&file);
			}
		}
		k_mutex_unlock(&g_sd_mtx);
	}

}

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
	if (!log_tid) {
		log_tid = k_thread_create(&log_thread_data, log_stack,
				K_THREAD_STACK_SIZEOF(log_stack),
				log_thread, NULL, NULL, NULL, 4, 0, K_NO_WAIT);
		shell_print(sh, "Loggerr started (period=%d ms).", FLUSH_PERIOD_MS);
	}

	return 0;
}
static int cmd_stop_sensors(const struct shell *sh, size_t argc, char **argv)
{
        ARG_UNUSED(argc); ARG_UNUSED(argv);

        if (log_tid) { k_thread_abort(log_tid); log_tid = NULL; shell_print(sh, "logger stopped."); }
        if (hum_tid)   { k_thread_abort(hum_tid);   hum_tid   = NULL; shell_print(sh, "HT worker stopped."); }
        if (press_tid) { k_thread_abort(press_tid); press_tid = NULL; shell_print(sh, "PRESS worker stopped."); }
        if (imu_tid)   { k_thread_abort(imu_tid);   imu_tid   = NULL; shell_print(sh, "IMU worker stopped."); }

        /* drain sems for a clean next start */
        while (k_sem_count_get(&semHT)    > 0) k_sem_take(&semHT,    K_NO_WAIT);
        while (k_sem_count_get(&semPress) > 0) k_sem_take(&semPress, K_NO_WAIT);
        while (k_sem_count_get(&semGyro)  > 0) k_sem_take(&semGyro,  K_NO_WAIT);

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

SHELL_STATIC_SUBCMD_SET_CREATE(sub_sensors,
        SHELL_CMD(start_sensors,        NULL, "Start periodic sensor logging",  cmd_start_sensor),
        SHELL_CMD(stop_sensors,         NULL, "Stop sensor logging",            cmd_stop_sensors),
        SHELL_CMD(clear_logs,           NULL, "Clear sensor log file",          cmd_clear_logs),
        SHELL_SUBCMD_SET_END
);
SHELL_CMD_REGISTER(sensors, &sub_sensors, "Sensor logging commands", NULL);

