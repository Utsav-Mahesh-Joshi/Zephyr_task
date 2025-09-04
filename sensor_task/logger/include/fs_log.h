#ifndef FS_LOG_H
#define FS_LOG_H

#include <zephyr/kernel.h>
#include <zephyr/fs/fs.h>

int fslog_init(void);
int fslog_append(const char *line);
int fslog_cat(size_t max_bytes);	/* print to shell/console */
int fslog_clear(void);

#endif

