#include <stdarg.h>
#include "my_console.h"

void my_console_print(const char *msg)
{
	printk("%s\n",msg);
}

void my_console_printf(const char *fmt,...)
{
	va_list args;
	va_start(args,fmt);
	vprintf(fmt,args);
	va_end(args);	
}





