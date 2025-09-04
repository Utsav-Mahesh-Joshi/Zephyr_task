#include "fs_log.h"
FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(lfs_sens);

LOG_MODULE_REGISTER(fs_log);

static struct fs_mount_t mount_lfs={
        .type=FS_LITTLEFS,
        .fs_data=&lfs_sens,
        .storage_dev=(void*)FIXED_PARTITION_ID(app_lfs),
        .mnt_point="/lfs",

};


static void mount_fs(struct fs_mount_t *mp)
{
    int rc = fs_mount(mp);
    if (rc == 0) {
        LOG_INF("Mounted at %s", mp->mnt_point);
    } else {
        LOG_ERR("Failed to mount %s (%d)", mp->mnt_point, rc);
    }
}
void mount_sens()
{
	mount_fs(&mount_lfs);
}

