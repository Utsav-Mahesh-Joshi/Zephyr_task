#ifndef FS_LOG_H
#define FS_LOG_H

#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/storage/flash_map.h>

/**
 * @file fs_log.h
 * @brief Filesystem mount helper interface for LittleFS.
 *
 * This module provides a wrapper for mounting the sensor
 * partition (`/lfs`) on a fixed flash partition using LittleFS.
 * It ensures filesystem is mounted before read/write operations.
 */

/**
 * @brief Mount the sensor filesystem partition.
 *
 * Initializes and mounts the `/lfs` partition configured to use
 * LittleFS on the `app_lfs` fixed flash partition.
 *
 * Example usage:
 * @code
 *  #include "fs_log.h"
 *
 *  void main(void)
 *  {
 *      mount_sens();
 *      // Now /lfs can be accessed using fs_open(), fs_read(), etc.
 *  }
 * @endcode
 *
 * @return void
 */
void mount_sens(void);

#endif /* FS_LOG_H */

