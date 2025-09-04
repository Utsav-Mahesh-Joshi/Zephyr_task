#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define STACK_SIZE 512  /**< Stack size for each thread. */
#define PRIO       5    /**< Thread priority. */

/** @brief Thread stack allocation for thread A. */
K_THREAD_STACK_DEFINE(threadA, STACK_SIZE);
/** @brief Thread stack allocation for thread B. */
K_THREAD_STACK_DEFINE(threadB, STACK_SIZE);
/** @brief Thread stack allocation for thread C. */
K_THREAD_STACK_DEFINE(threadC, STACK_SIZE);

/** @brief Thread control block for thread A. */
static struct k_thread threadA_data;
/** @brief Thread control block for thread B. */
static struct k_thread threadB_data;
/** @brief Thread control block for thread C. */
static struct k_thread threadC_data;

/** @brief Condition variable for thread coordination. */
static struct k_condvar cond;
/** @brief Mutex used with condition variable to protect shared state. */
static struct k_mutex mutex;

/** 
 * @brief Shared state to track which thread should execute.  
 * Values cycle from 0 → 8, controlling turn-taking between threads.  
 */
uint8_t ready = 0;

/**
 * @brief Function executed by thread A.
 *
 * Prints elements from array `{1, 4, 7}` when `ready % 3 == 0`.  
 * Uses condition variables to wait for its turn and signals the next thread after printing.
 *
 * @param a Unused parameter.  
 * @param b Unused parameter.  
 * @param c Unused parameter.  
 *
 * @return void
 */
void threadA_fn(void *a, void *b, void *c)
{
	int8_t arr[] = {1, 4, 7};

	while (1) {
		k_mutex_lock(&mutex, K_FOREVER);
		while (ready % 3 != 0) {
			k_condvar_wait(&cond, &mutex, K_FOREVER);
		}
		printk("A:%d\r\n", arr[ready / 3]);
		ready = (ready + 1) % 9;
		k_condvar_signal(&cond);
		k_mutex_unlock(&mutex);
		k_msleep(500);
	}
}

/**
 * @brief Function executed by thread B.
 *
 * Prints elements from array `{2, 5, 8}` when `ready % 3 == 1`.  
 * Uses condition variables to wait for its turn and signals the next thread after printing.
 *
 * @param a Unused parameter.  
 * @param b Unused parameter.  
 * @param c Unused parameter.  
 *
 * @return void
 */
void threadB_fn(void *a, void *b, void *c)
{
	int8_t arr[] = {2, 5, 8};

	while (1) {
		k_mutex_lock(&mutex, K_FOREVER);
		while (ready % 3 != 1) {
			k_condvar_wait(&cond, &mutex, K_FOREVER);
		}
		printk("B:%d\r\n", arr[ready / 3]);
		ready = (ready + 1) % 9;
		k_condvar_signal(&cond);
		k_mutex_unlock(&mutex);
		k_msleep(500);
	}
}

/**
 * @brief Function executed by thread C.
 *
 * Prints elements from array `{3, 6, 9}` when `ready % 3 == 2`.  
 * Uses condition variables to wait for its turn and signals the next thread after printing.
 *
 * @param a Unused parameter.  
 * @param b Unused parameter.  
 * @param c Unused parameter.  
 *
 * @return void
 */
void threadC_fn(void *a, void *b, void *c)
{
	int8_t arr[] = {3, 6, 9};

	while (1) {
		k_mutex_lock(&mutex, K_FOREVER);
		while (ready % 3 != 2) {
			k_condvar_wait(&cond, &mutex, K_FOREVER);
		}
		printk("C:%d\r\n", arr[ready / 3]);
		ready = (ready + 1) % 9;
		k_condvar_signal(&cond);
		k_mutex_unlock(&mutex);
		k_msleep(500);
	}
}

/**
 * @brief Main entry point of the application.
 *
 * - Initializes the mutex and condition variable.  
 * - Creates three threads (`threadA_fn`, `threadB_fn`, `threadC_fn`).  
 * - Threads coordinate execution using condition variables, ensuring ordered printing:
 *   ```
 *   A:1 → B:2 → C:3
 *   A:4 → B:5 → C:6
 *   A:7 → B:8 → C:9
 *   ```
 * - Sequence repeats indefinitely.
 *
 * @retval 0 Always returns 0 in Zephyr (program runs indefinitely).
 */
int main(void)
{
	k_mutex_init(&mutex);
	k_condvar_init(&cond);

	k_thread_create(&threadA_data, threadA, STACK_SIZE,
			threadA_fn, NULL, NULL, NULL,
			PRIO, 0, K_NO_WAIT);

	k_thread_create(&threadB_data, threadB, STACK_SIZE,
			threadB_fn, NULL, NULL, NULL,
			PRIO, 0, K_NO_WAIT);

	k_thread_create(&threadC_data, threadC, STACK_SIZE,
			threadC_fn, NULL, NULL, NULL,
			PRIO, 0, K_NO_WAIT);
}

