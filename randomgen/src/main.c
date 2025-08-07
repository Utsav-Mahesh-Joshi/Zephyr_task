#include <zephyr/random/radom.h>

int main(void)
{
	uint32_t rnd;
	while(1)
	{
	rnd=sys_rand32_get();
	printk("Random value :%u\r\n",rnd)
	k_msleep(1000);
	}
	return 0;
}
