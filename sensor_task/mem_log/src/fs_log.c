#include "fs_log.h"

FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(lfs_sens);

/** @brief Register log module for filesystem operations. */
LOG_MODULE_REGISTER(fs_log);

/**
 * @brief Filesystem mount configuration.
 *
 * - Uses LittleFS backend.  
 * - Mount point: `/lfs`.  
 * - Storage device: fixed partition `app_lfs`.  
 */
static struct fs_mount_t mount_lfs = {
	.type = FS_LITTLEFS,
	.fs_data = &lfs_sens,
	.storage_dev = (void *)FIXED_PARTITION_ID(app_lfs),
	.mnt_point = "/lfs",
};

/**
 * @brief Mount a filesystem and log the result.
 *
 * Calls @ref fs_mount with the provided mount configuration and logs
 * the result at INFO or ERROR level.
 *
 * @param mp Pointer to filesystem mount configuration.
 *
 * @return void
 */
static void mount_fs(struct fs_mount_t *mp)
{
	int rc = fs_mount(mp);
	if (rc == 0) {
		LOG_INF("Mounted at %s", mp->mnt_point);
	} else {
		LOG_ERR("Failed to mount %s (%d)", mp->mnt_point, rc);
	}
}

/**
 * @brief Mount the sensor filesystem partition.
 *
 * Convenience wrapper that mounts the `/lfs` partition defined
 * in @ref mount_lfs. Must be called before any read/write operations
 * on the LittleFS partition.
 *
 * @return void
 */
void mount_sens(void)
{
	mount_fs(&mount_lfs);
}

