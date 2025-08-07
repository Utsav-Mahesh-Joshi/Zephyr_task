#ifndef MY_CONSOLE_H
#define MY_CONSOLE_H

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

void my_console_print(const char *msg);
void my_console_printf(const char *fmt,...);

#endif
