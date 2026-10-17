#include <stdint.h>
#include <pthread.h>
#include <stdatomic.h>
#include <genmc.h>

/*
 * As spin-cas-writeback-live0, with the expected value behind a pointer
 * parameter (a ring buffer's move-head loop): after inlining it is the same
 * header PHI fed by the cmpxchg result.
 */

#define CAPACITY 4

atomic_uint prod_head;
atomic_uint cons_tail;

static uint32_t move_prod_head(uint32_t n, uint32_t *old_head, uint32_t *new_head)
{
	*old_head = atomic_load_explicit(&prod_head, memory_order_relaxed);
	do {
		uint32_t free_entries =
			CAPACITY + atomic_load_explicit(&cons_tail, memory_order_acquire) - *old_head;
		if (n > free_entries)
			return 0;
		*new_head = *old_head + n;
	} while (!atomic_compare_exchange_strong_explicit(&prod_head, old_head, *new_head,
							  memory_order_relaxed, memory_order_relaxed));
	return n;
}

static void *producer(void *unused)
{
	uint32_t old_head, new_head;
	move_prod_head(1, &old_head, &new_head);
	return NULL;
}

int main()
{
	pthread_t a, b;
	pthread_create(&a, NULL, producer, NULL);
	pthread_create(&b, NULL, producer, NULL);
	return 0;
}
