#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define STACK_SIZE 512  /**< Stack size for each thread. */
#define PRIO       5    /**< Thread priority for both threads. */

/** @brief Thread stack allocation for thread A. */
K_THREAD_STACK_DEFINE(threadA_stack, STACK_SIZE);
/** @brief Thread stack allocation for thread B. */
K_THREAD_STACK_DEFINE(threadB_stack, STACK_SIZE);

/** @brief Thread control block for thread A. */
static struct k_thread threadA_data;
/** @brief Thread control block for thread B. */
static struct k_thread threadB_data;

/**
 * @brief Binary semaphore used for synchronization.
 *
 * - Initial count: 0 → thread A blocks immediately.  
 * - Maximum count: 1 → acts as a binary semaphore.  
 */
K_SEM_DEFINE(my_sem, 0, 1);

/**
 * @brief Function executed by thread A.
 *
 * - Prints message while waiting for semaphore.  
 * - Calls `k_sem_take()` and blocks until semaphore is given.  
 * - When unblocked, prints confirmation message.  
 * - Repeats indefinitely.  
 *
 * @param a Unused parameter.  
 * @param b Unused parameter.  
 * @param c Unused parameter.  
 *
 * @return void
 */
void threadA_fn(void *a, void *b, void *c)
{
	while (1) {
		printk("A: waiting for semaphore \r\n");
		k_sem_take(&my_sem, K_FOREVER);
		printk("A: got semaphore \r\n");
	}
}

/**
 * @brief Function executed by thread B.
 *
 * - Sleeps for 3000 ms.  
 * - Gives semaphore using `k_sem_give()` to unblock thread A.  
 * - Repeats indefinitely.  
 *
 * @param a Unused parameter.  
 * @param b Unused parameter.  
 * @param c Unused parameter.  
 *
 * @return void
 */
void threadB_fn(void *a, void *b, void *c)
{
	while (1) {
		k_msleep(3000);
		printk("B: giving semaphore\r\n");
		k_sem_give(&my_sem);
	}
}

/**
 * @brief Main entry point of the application.
 *
 * - Prints startup message.  
 * - Creates two threads:
 *   - `threadA_fn`: waits on the semaphore.  
 *   - `threadB_fn`: periodically releases the semaphore.  
 * - Demonstrates **thread synchronization via semaphore signaling**.  
 *
 * Expected output:
 * ```
 * starting main task
 * A: waiting for semaphore
 * B: giving semaphore
 * A: got semaphore
 * A: waiting for semaphore
 * ...
 * ```
 *
 * @retval 0 Always returns 0 in Zephyr (program runs indefinitely).
 */
int main(void)
{
	printk("starting main task\r\n");

	k_thread_create(&threadA_data, threadA_stack, STACK_SIZE,
			threadA_fn, NULL, NULL, NULL,
			PRIO, 0, K_NO_WAIT);

	k_thread_create(&threadB_data, threadB_stack, STACK_SIZE,
			threadB_fn, NULL, NULL, NULL,
			PRIO, 0, K_NO_WAIT);

	return 0;
}

