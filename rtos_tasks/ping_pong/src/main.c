#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define STACK_SIZE 512  /**< Stack size for each thread. */
#define PRIO       5    /**< Thread priority. */
#define ITERS      100  /**< Number of ping-pong iterations. */

/** @brief Thread stack allocation for thread A. */
K_THREAD_STACK_DEFINE(threadA_stack, STACK_SIZE);
/** @brief Thread stack allocation for thread B. */
K_THREAD_STACK_DEFINE(threadB_stack, STACK_SIZE);

/** @brief Thread control block for thread A. */
static struct k_thread threadA_data;
/** @brief Thread control block for thread B. */
static struct k_thread threadB_data;

/** 
 * @brief Semaphore for thread A.  
 * Initialized with count 1 so thread A starts first. 
 */
K_SEM_DEFINE(semA, 1, 1);

/** 
 * @brief Semaphore for thread B.  
 * Initialized with count 0 so thread B waits until thread A signals. 
 */
K_SEM_DEFINE(semB, 0, 1);

/**
 * @brief Function executed by thread A.
 *
 * - Waits on @ref semA.  
 * - Prints `"ping"` with iteration index.  
 * - Signals @ref semB to wake thread B.  
 * - Repeats @ref ITERS times.  
 *
 * @param a Unused parameter.  
 * @param b Unused parameter.  
 * @param c Unused parameter.  
 *
 * @return void
 */
static void threadA_fn(void *a, void *b, void *c)
{
	for (uint16_t i = 0; i < ITERS; i++) {
		k_sem_take(&semA, K_FOREVER);
		printk("ping :%d\r\n", i);
		k_sem_give(&semB);
		k_msleep(100);
	}
	printk("threadA end\n");
}

/**
 * @brief Function executed by thread B.
 *
 * - Waits on @ref semB.  
 * - Prints `"pong"` with iteration index.  
 * - Signals @ref semA to wake thread A.  
 * - Repeats @ref ITERS times.  
 *
 * @param a Unused parameter.  
 * @param b Unused parameter.  
 * @param c Unused parameter.  
 *
 * @return void
 */
static void threadB_fn(void *a, void *b, void *c)
{
	for (uint16_t i = 0; i < ITERS; i++) {
		k_sem_take(&semB, K_FOREVER);
		printk("pong :%d\r\n", i);
		k_sem_give(&semA);
		k_msleep(100);
	}
	printk("threadB end\n");
}

/**
 * @brief Main entry point of the application.
 *
 * - Creates two threads (`threadA_fn` and `threadB_fn`).  
 * - Threads synchronize using semaphores to alternately print
 *   `"ping"` and `"pong"`.  
 * - Output format:
 *   ```
 *   ping :0
 *   pong :0
 *   ping :1
 *   pong :1
 *   ...
 *   ```
 * - Runs until both threads finish their iterations.  
 *
 * @retval 0 Always returns 0 in Zephyr (program runs indefinitely).
 */
int main(void)
{
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

