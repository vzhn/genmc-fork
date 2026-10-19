#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>

/*
 * DPDK's MCS lock (rte_mcslock_lock()/rte_mcslock_unlock()), with the
 * orders of the current code. A thread sets locked and clears next in its
 * own node, swaps the node into the tail and, behind a predecessor, links
 * itself into the predecessor's next and waits until its locked is
 * cleared. Unlock clears the successor's locked; with no successor in
 * sight it tries to swing the tail back to null, and if that fails it
 * waits for the successor's link first.
 *
 * NTHREADS threads take the lock once each; the critical sections never
 * overlap and nobody waits forever.
 *
 * BUG_RELAXED_LINK: the link into the predecessor is a relaxed store, as
 * in DPDK 20.05. The exchange between the locked = 1 store and the link
 * does not order them: its release covers only its own write. The
 * predecessor can follow the link and clear locked before the locked = 1
 * store lands, which then overwrites the clear, and the thread waits
 * forever. VSync found this (Oberhauser et al., ASPLOS 2021, Sec. 6.2);
 * DPDK patch 75983 (August 2020) made the link a release store.
 */

#ifndef NTHREADS
# define NTHREADS 2
#endif

#ifdef BUG_RELAXED_LINK
# define LINK_ORDER memory_order_relaxed
#else
# define LINK_ORDER memory_order_release
#endif

struct mcs_node {
	_Atomic(struct mcs_node *) next;
	atomic_int locked;
};

_Atomic(struct mcs_node *) tail;
struct mcs_node nodes[NTHREADS];
atomic_int in_cs;

static void mcslock_lock(struct mcs_node *me)
{
	atomic_store_explicit(&me->locked, 1, memory_order_relaxed);
	atomic_store_explicit(&me->next, NULL, memory_order_relaxed);

	struct mcs_node *prev = atomic_exchange_explicit(&tail, me, memory_order_acq_rel);

	if (prev == NULL)
		return;
	atomic_store_explicit(&prev->next, me, LINK_ORDER);
	__VERIFIER_await(atomic_load_explicit(&me->locked, memory_order_acquire) == 0);
}

static void mcslock_unlock(struct mcs_node *me)
{
	if (atomic_load_explicit(&me->next, memory_order_acquire) == NULL) {
		struct mcs_node *expected = me;

		if (atomic_compare_exchange_strong_explicit(&tail, &expected, NULL,
							    memory_order_release,
							    memory_order_relaxed))
			return;
		/* a successor is linking itself in */
		__VERIFIER_await(atomic_load_explicit(&me->next, memory_order_acquire) != NULL);
	}
	struct mcs_node *next = atomic_load_explicit(&me->next, memory_order_acquire);

	atomic_store_explicit(&next->locked, 0, memory_order_release);
}

static void *thread(void *arg)
{
	struct mcs_node *me = &nodes[(intptr_t) arg];

	mcslock_lock(me);
	int others = atomic_fetch_add_explicit(&in_cs, 1, memory_order_relaxed);
	assert(others == 0);
	atomic_fetch_sub_explicit(&in_cs, 1, memory_order_relaxed);
	mcslock_unlock(me);
	return NULL;
}

int main()
{
	pthread_t t[NTHREADS];

	for (intptr_t i = 0; i < NTHREADS; i++)
		if (pthread_create(&t[i], NULL, thread, (void *) i))
			abort();
	for (int i = 0; i < NTHREADS; i++)
		if (pthread_join(t[i], NULL))
			abort();
	return 0;
}
