#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>

/*
 * Restarting a stopped blk-mq hardware queue while a request is added to
 * it (block/blk-mq.c, block/blk-mq.h). The submitter adds a request to the
 * dispatch list and runs the queue, which dispatches only if the queue is
 * not stopped (blk_mq_hctx_stopped()). The driver's completion path
 * restarts the queue: it clears BLK_MQ_S_STOPPED and runs the queue, which
 * dispatches only if a request is pending. Kernel primitives map to C11:
 * test_bit()/clear_bit() and the list accesses are relaxed, smp_mb() and
 * smp_mb__after_atomic() seq_cst fences. Taking a request off the list is
 * an exchange, so it is dispatched once.
 *
 * The queue starts stopped. Whoever runs it last must find the request:
 * it is always dispatched.
 *
 * BUG_NO_BARRIERS: no smp_mb() in the slow path of blk_mq_hctx_stopped()
 * and no smp_mb__after_atomic() after clearing the bit, as before commit
 * 96a9fe64bfd4 ("block: fix ordering between checking BLK_MQ_S_STOPPED
 * request adding", 2024). This is store buffering: the submitter still
 * sees the queue stopped, the restarter does not see the request, and the
 * request is never dispatched. Passes under SC.
 */

#define BLK_MQ_S_STOPPED 1UL

atomic_ulong hctx_state;
atomic_int dispatch_list;	/* number of requests on the list */
atomic_int dispatched;

static bool blk_mq_hctx_stopped(void)
{
	if (!(atomic_load_explicit(&hctx_state, memory_order_relaxed) & BLK_MQ_S_STOPPED))
		return false;
#ifndef BUG_NO_BARRIERS
	atomic_thread_fence(memory_order_seq_cst);
#endif
	return atomic_load_explicit(&hctx_state, memory_order_relaxed) & BLK_MQ_S_STOPPED;
}

static void dispatch_requests(void)
{
	int n = atomic_exchange_explicit(&dispatch_list, 0, memory_order_relaxed);

	atomic_fetch_add_explicit(&dispatched, n, memory_order_relaxed);
}

static void *submitter(void *arg)
{
	/* blk_mq_request_bypass_insert() */
	atomic_fetch_add_explicit(&dispatch_list, 1, memory_order_relaxed);
	/* blk_mq_run_hw_queue() */
	if (blk_mq_hctx_stopped())
		return NULL;
	dispatch_requests();
	return NULL;
}

static void *restarter(void *arg)
{
	/* blk_mq_start_stopped_hw_queue() */
	atomic_fetch_and_explicit(&hctx_state, ~BLK_MQ_S_STOPPED, memory_order_relaxed);
#ifndef BUG_NO_BARRIERS
	atomic_thread_fence(memory_order_seq_cst);
#endif
	/* blk_mq_run_hw_queue(): blk_mq_hctx_has_pending() */
	if (atomic_load_explicit(&dispatch_list, memory_order_relaxed) == 0)
		return NULL;
	if (blk_mq_hctx_stopped())
		return NULL;
	dispatch_requests();
	return NULL;
}

int main()
{
	pthread_t s, r;

	atomic_init(&hctx_state, BLK_MQ_S_STOPPED);
	if (pthread_create(&s, NULL, submitter, NULL) ||
	    pthread_create(&r, NULL, restarter, NULL))
		abort();
	if (pthread_join(s, NULL) || pthread_join(r, NULL))
		abort();
	assert(atomic_load_explicit(&dispatched, memory_order_relaxed) == 1);
	return 0;
}
