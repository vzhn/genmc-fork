#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>

/*
 * The CLH big kernel lock of seL4 (clh_lock_acquire()/clh_lock_release()),
 * reduced to its ordering. Each thread owns a queue node whose state is
 * Pending or Granted. To acquire, a thread marks its node Pending, swaps
 * it into the queue tail and waits until the predecessor's node is
 * Granted. To release, it marks its node Granted and takes over the
 * predecessor's node for its next acquisition.
 *
 * NTHREADS threads take the lock once each; the critical sections never
 * overlap and nobody waits forever.
 *
 * BUG_EXCHANGE_ACQUIRE: the exchange that enqueues the node has only
 * acquire order, so the Pending store before it is not released with it.
 * The next thread can find the node in the tail, still read its old
 * Granted state and enter the critical section while the owner is still
 * in it. VSync found this (Oberhauser et al., ASPLOS 2021, Sec. 6.1) and
 * reproduced it on Arm hardware; seL4 PR #199 made the exchange
 * acquire-release.
 */

#ifndef NTHREADS
# define NTHREADS 2
#endif

#define GRANTED 0
#define PENDING 1

#ifdef BUG_EXCHANGE_ACQUIRE
# define ENQUEUE_ORDER memory_order_acquire
#else
# define ENQUEUE_ORDER memory_order_acq_rel
#endif

struct clh_node {
	atomic_int state;
};

struct clh_node pool[NTHREADS + 1];	/* pool[NTHREADS]: the initial tail */
_Atomic(struct clh_node *) tail;
struct clh_node *my_node[NTHREADS];
struct clh_node *my_pred[NTHREADS];
atomic_int in_cs;

static void clh_lock_acquire(int me)
{
	struct clh_node *node = my_node[me];

	atomic_store_explicit(&node->state, PENDING, memory_order_relaxed);
	struct clh_node *pred = atomic_exchange_explicit(&tail, node, ENQUEUE_ORDER);
	my_pred[me] = pred;
	__VERIFIER_await(atomic_load_explicit(&pred->state, memory_order_acquire) == GRANTED);
}

static void clh_lock_release(int me)
{
	atomic_store_explicit(&my_node[me]->state, GRANTED, memory_order_release);
	my_node[me] = my_pred[me];
}

static void *thread(void *arg)
{
	int me = (intptr_t) arg;

	clh_lock_acquire(me);
	int others = atomic_fetch_add_explicit(&in_cs, 1, memory_order_relaxed);
	assert(others == 0);
	atomic_fetch_sub_explicit(&in_cs, 1, memory_order_relaxed);
	clh_lock_release(me);
	return NULL;
}

int main()
{
	pthread_t t[NTHREADS];

	atomic_init(&pool[NTHREADS].state, GRANTED);
	atomic_init(&tail, &pool[NTHREADS]);
	for (int i = 0; i < NTHREADS; i++)
		my_node[i] = &pool[i];
	for (intptr_t i = 0; i < NTHREADS; i++)
		if (pthread_create(&t[i], NULL, thread, (void *) i))
			abort();
	for (int i = 0; i < NTHREADS; i++)
		if (pthread_join(t[i], NULL))
			abort();
	return 0;
}
