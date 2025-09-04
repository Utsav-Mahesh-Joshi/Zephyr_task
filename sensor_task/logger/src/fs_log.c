#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/fs/fs.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(fslog, LOG_LEVEL_INF);

#define LOG_PATH	"/lfs/senslog.csv"

static struct fs_mount_t lfs_mnt = {
	.type = FS_LITTLEFS,
	.mnt_point = "/lfs",
	.fs_data = NULL,
};

int fslog_init(void)
{
	int rc;

	rc = fs_mount(&lfs_mnt);
	if (rc != 0 && rc != -EEXIST) {
		LOG_ERR("mount failed: %d", rc);
		return rc;
	}

	/* ensure file exists with header */
	struct fs_file_t f;
	fs_file_t_init(&f);
	rc = fs_open(&f, LOG_PATH, FS_O_CREATE | FS_O_READ | FS_O_WRITE);
	if (rc) {
		LOG_ERR("open header: %d", rc);
		return rc;
	}

	/* write header if empty */
	struct fs_dirent ent;
	rc = fs_stat(LOG_PATH, &ent);
	if (rc == 0 && ent.size == 0) {
		const char *hdr = "ts_ms,temp_c,hum_pct,press_hpa,ax,ay,az\r\n";
		(void)fs_write(&f, hdr, strlen(hdr));
	}
	fs_close(&f);
	return 0;
}

int fslog_append(const char *line)
{
	struct fs_file_t f;
	int rc;

	fs_file_t_init(&f);
	rc = fs_open(&f, LOG_PATH, FS_O_WRITE | FS_O_APPEND);
	if (rc) {
		LOG_ERR("append open: %d", rc);
		return rc;
	}
	rc = fs_write(&f, line, strlen(line));
	fs_close(&f);
	return rc < 0 ? rc : 0;
}

int fslog_cat(size_t max_bytes)
{
	struct fs_file_t f;
	int rc;

	fs_file_t_init(&f);
	rc = fs_open(&f, LOG_PATH, FS_O_READ);
	if (rc) {
		LOG_ERR("cat open: %d", rc);
		return rc;
	}

	char buf[256];
	size_t left = max_bytes;
	ssize_t rd;

	while (left > 0 && (rd = fs_read(&f, buf, MIN(left, sizeof(buf)))) > 0) {
		printk("%.*s", (int)rd, buf);
		left -= rd;
	}

	fs_close(&f);
	return 0;
}

int fslog_clear(void)
{
	/* truncate by re-creating */
	int rc = fs_unlink(LOG_PATH);
	if (rc && rc != -ENOENT) {
		return rc;
	}
	return fslog_init();
}

