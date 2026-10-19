#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>

/*
 * Queueing work that may already be pending (kernel/workqueue.c), the
 * pattern that relies on test_and_set_bit() being ordered even when the
 * bit is already set. A producer stores new data and calls queue_work(),
 * which does test_and_set_bit(WORK_STRUCT_PENDING): if the bit was clear,
 * the work is queued again; if it was set, the queued instance will run
 * and is expected to see the data. The worker, before running the work
 * function, clears the pending bit and issues smp_mb(), then the work
 * function reads the data. Kernel primitives map to C11: test_and_set_bit()
 * is a relaxed fetch_or between two seq_cst fences (fully ordered), the
 * clear and the data accesses are relaxed, smp_mb() a seq_cst fence.
 *
 * The work is pending at the start. Either the producer queues it again,
 * or the running instance sees the new data: an update is never lost.
 *
 * BUG_EARLY_EXIT: test_and_set_bit() first reads the word with READ_ONCE()
 * and returns early if the bit is already set, as the generic bitops did
 * until commit 415d83249709 ("locking/atomic: Make test_and_*_bit()
 * ordered on failure", 2022). That load is unordered, so with the worker
 * this is store buffering: the producer can see the stale pending bit
 * while the worker reads the old data. On Apple M1 this lost TTY input.
 * Passes under SC.
 */

#define WORK_PENDING 1UL

atomic_ulong work_data;		/* the pending bit */
atomic_int data;
atomic_bool requeued;
int seen;

static bool test_and_set_bit(atomic_ulong *word, unsigned long mask)
{
#ifdef BUG_EARLY_EXIT
	if (atomic_load_explicit(word, memory_order_relaxed) & mask)
		return true;
#endif
	atomic_thread_fence(memory_order_seq_cst);
	unsigned long old = atomic_fetch_or_explicit(word, mask, memory_order_relaxed);
	atomic_thread_fence(memory_order_seq_cst);
	return old & mask;
}

static void *producer(void *arg)
{
	atomic_store_explicit(&data, 1, memory_order_relaxed);
	/* queue_work() */
	if (!test_and_set_bit(&work_data, WORK_PENDING))
		atomic_store_explicit(&requeued, true, memory_order_relaxed);
	return NULL;
}

static void *worker(void *arg)
{
	/* process_one_work(): set_work_pool_and_clear_pending(), smp_mb() */
	atomic_fetch_and_explicit(&work_data, ~WORK_PENDING, memory_order_relaxed);
	atomic_thread_fence(memory_order_seq_cst);
	/* the work function */
	seen = atomic_load_explicit(&data, memory_order_relaxed);
	return NULL;
}

int main()
{
	pthread_t p, w;

	atomic_init(&work_data, WORK_PENDING);
	if (pthread_create(&p, NULL, producer, NULL) ||
	    pthread_create(&w, NULL, worker, NULL))
		abort();
	if (pthread_join(p, NULL) || pthread_join(w, NULL))
		abort();
	assert(atomic_load_explicit(&requeued, memory_order_relaxed) || seen == 1);
	return 0;
}
