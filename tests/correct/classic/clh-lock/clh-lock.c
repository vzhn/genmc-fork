#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>

/*
 * The CLH queue lock (The Art of Multiprocessor Programming, Figs. 7.9 and
 * 7.10). A thread sets its node's locked flag, swaps the node into tail and
 * spins on its predecessor's node. Unlock clears the flag of its own node
 * and takes the predecessor's node as its node for the next acquisition:
 * the successor may still be spinning on the old one. Spins are awaits.
 *
 * T1 takes the lock twice, T2 once; the critical sections never overlap and
 * nobody waits forever (-check-liveness).
 *
 * BUG_REUSE_NODE: unlock keeps the thread's own node (the incorrect CLH lock
 * of Fig. 7.33). When T1 locks again before T2, spinning on T1's node, has
 * seen it unlocked, T1 sets the flag again and queues behind T2: T2 waits
 * for T1 and T1 for T2.
 */

#define NTHREADS 2

struct qnode {
	atomic_bool locked;
};

struct qnode pool[NTHREADS + 1];	/* pool[NTHREADS]: the initial tail */
_Atomic(struct qnode *) tail;
struct qnode *my_node[NTHREADS];
struct qnode *my_pred[NTHREADS];
atomic_int in_cs;

static void clh_lock(int me)
{
	struct qnode *n = my_node[me];

	atomic_store_explicit(&n->locked, true, memory_order_relaxed);

	struct qnode *pred = atomic_exchange_explicit(&tail, n, memory_order_acq_rel);

	my_pred[me] = pred;
	__VERIFIER_await(!atomic_load_explicit(&pred->locked, memory_order_acquire));
}

static void clh_unlock(int me)
{
	atomic_store_explicit(&my_node[me]->locked, false, memory_order_release);
#ifndef BUG_REUSE_NODE
	my_node[me] = my_pred[me];
#endif
}

static void critical_section(void)
{
	int others = atomic_fetch_add_explicit(&in_cs, 1, memory_order_relaxed);

	assert(others == 0);
	atomic_fetch_sub_explicit(&in_cs, 1, memory_order_relaxed);
}

static void *thread(void *arg)
{
	int me = (intptr_t) arg;
	int rounds = me == 0 ? 2 : 1;

	for (int i = 0; i < rounds; i++) {
		clh_lock(me);
		critical_section();
		clh_unlock(me);
	}
	return NULL;
}

int main()
{
	pthread_t t[NTHREADS];

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
