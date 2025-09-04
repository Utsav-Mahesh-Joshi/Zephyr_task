#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define STACK_SIZE 512  /**< Stack size for each thread. */
#define PRIO       5    /**< Priority for both threads. */

/** @brief Shared counter between threads. */
int32_t count = 0;

/** @brief Stack memory for thread A. */
K_THREAD_STACK_DEFINE(threadA_stack, STACK_SIZE);

/** @brief Stack memory for thread B. */
K_THREAD_STACK_DEFINE(threadB_stack, STACK_SIZE);

/** @brief Thread control block for thread A. */
static struct k_thread threadA_data;

/** @brief Thread control block for thread B. */
static struct k_thread threadB_data;

/** 
 * @brief Mutex for protecting access to shared variable @ref count. 
 */
static struct k_mutex mutex;

/**
 * @brief Function executed by thread A.
 *
 * - Prints the current value of @ref count.  
 * - Locks the mutex, increments @ref count, then unlocks it.  
 * - Repeats 100 times with 100 ms delay between iterations.  
 *
 * @param a Unused parameter.  
 * @param b Unused parameter.  
 * @param c Unused parameter.  
 *
 * @return void
 */
void threadA_fn(void *a, void *b, void *c)
{
	for (uint16_t i = 0; i < 100; i++) {
		printk("threadA count %d\r\n", count);
		k_mutex_lock(&mutex, K_FOREVER);
		count++;
		k_mutex_unlock(&mutex);
		k_msleep(100);
	}
	printk("threadA end\r\n");
	printk("threadA count %d\r\n", count);
}

/**
 * @brief Function executed by thread B.
 *
 * - Prints the current value of @ref count.  
 * - Locks the mutex, increments @ref count, then unlocks it.  
 * - Repeats 100 times with 100 ms delay between iterations.  
 *
 * @param a Unused parameter.  
 * @param b Unused parameter.  
 * @param c Unused parameter.  
 *
 * @return void
 */
void threadB_fn(void *a, void *b, void *c)
{
	for (uint16_t i = 0; i < 100; i++) {
		printk("threadB count %d\r\n", count);
		k_mutex_lock(&mutex, K_FOREVER);
		count++;
		k_mutex_unlock(&mutex);
		k_msleep(100);
	}
	printk("threadB end\r\n");
	printk("threadB count %d\r\n", count);
}

/**
 * @brief Main entry point of the application.
 *
 * - Initializes the mutex.  
 * - Creates two threads (`threadA_fn` and `threadB_fn`).  
 * - Main thread sleeps indefinitely while worker threads run.  
 *
 * @retval 0 Always returns 0 in Zephyr (infinite loop).
 */
int main(void)
{
	printk("starting main task\r\n");
	k_mutex_init(&mutex);

	k_thread_create(&threadA_data, threadA_stack, STACK_SIZE,
			threadA_fn, NULL, NULL, NULL,
			PRIO, 0, K_NO_WAIT);

	k_thread_create(&threadB_data, threadB_stack, STACK_SIZE,
			threadB_fn, NULL, NULL, NULL,
			PRIO, 0, K_NO_WAIT);

	while (1) {
		k_msleep(1000);
	}
}

