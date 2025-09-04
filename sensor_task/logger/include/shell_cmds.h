#ifndef SHELL_CMDS_H
#define SHELL_CMDS_H

#include <zephyr/shell/shell.h>

void sens_shell_register(void);
void sens_update_period_ms(uint32_t ms);
uint32_t sens_get_period_ms(void);
void sens_set_live(bool en);

#endif

