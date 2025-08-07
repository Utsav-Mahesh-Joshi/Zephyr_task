#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define STACK_SIZE 512
#define PRIO 5

K_THREAD_STACK_DEFINE(threadA,STACK_SIZE);
K_THREAD_STACK_DEFINE(threadB,STACK_SIZE);
K_THREAD_STACK_DEFINE(threadC,STACK_SIZE);

static struct k_thread threadA_data;
static struct k_thread threadB_data;
static struct k_thread threadC_data;

static struct k_condvar cond;
static struct k_mutex mutex;

uint8_t ready=0;

void threadA_fn(void *a,void *b,void *c)
{
	int8_t arr[]={1,4,7};

	
	while(1)
	{
		k_mutex_lock(&mutex,K_FOREVER);
		while(ready%3 !=0)
		{
			k_condvar_wait(&cond,&mutex,K_FOREVER);
		}
		printk("A:%d\r\n",arr[ready/3]);
		ready=(ready+1)%9;
		k_condvar_signal(&cond);
		k_mutex_unlock(&mutex);
		k_msleep(500);
	}

}
void threadB_fn(void *a,void *b,void *c)
{
	int8_t arr[]={2,5,8};

	
	while(1)
	{
		k_mutex_lock(&mutex,K_FOREVER);
		while(ready%3 !=1)
		{
			k_condvar_wait(&cond,&mutex,K_FOREVER);
		}
		printk("B:%d\r\n",arr[ready/3]);
		ready=(ready+1)%9;
		k_condvar_signal(&cond);
		k_mutex_unlock(&mutex);
		k_msleep(500);
	}

}

void threadC_fn(void *a,void *b,void *c)
{
	int8_t arr[]={3,6,9};

	
	while(1)
	{
		k_mutex_lock(&mutex,K_FOREVER);
		while(ready%3 !=2)
		{
			k_condvar_wait(&cond,&mutex,K_FOREVER);
		}
		printk("C:%d\r\n",arr[ready/3]);
		ready=(ready+1)%9;
		k_condvar_signal(&cond);
		k_mutex_unlock(&mutex);
		k_msleep(500);
	}

}

int main()
{
	k_mutex_init(&mutex);
	k_condvar_init(&cond);

	k_thread_create(&threadA_data,threadA,STACK_SIZE,threadA_fn,NULL,NULL,NULL,PRIO,0,K_NO_WAIT);
	k_thread_create(&threadB_data,threadB,STACK_SIZE,threadB_fn,NULL,NULL,NULL,PRIO,0,K_NO_WAIT);
	k_thread_create(&threadC_data,threadC,STACK_SIZE,threadC_fn,NULL,NULL,NULL,PRIO,0,K_NO_WAIT);
}
