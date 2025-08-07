#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define STACK_SIZE 512
#define PRIO 5

K_THREAD_STACK_DEFINE(threadA_stack,STACK_SIZE);
K_THREAD_STACK_DEFINE(threadB_stack,STACK_SIZE);

static struct k_thread threadA_data;
static struct k_thread threadB_data;

K_SEM_DEFINE(my_sem,0,1);

void threadA_fn(void *a, void *b, void *c)
{
	while(1)
	{
		printk("A: waiting for semaphore \r\n");
		k_sem_take(&my_sem,K_FOREVER);
		printk("A: got semaphore \r\n");
	}
}

void threadB_fn(void *a,void *b, void *c)
{
	while(1)
	{
		k_msleep(3000);
		printk("B:giving semaphore\r\n");
		k_sem_give(&my_sem);
	}
}

int main()
{
	printk("starting main task\r\n");
	k_thread_create(&threadA_data,threadA_stack,STACK_SIZE,threadA_fn,NULL,NULL,NULL,PRIO,0,K_NO_WAIT);
	k_thread_create(&threadB_data,threadB_stack,STACK_SIZE,threadB_fn,NULL,NULL,NULL,PRIO,0,K_NO_WAIT);

}
