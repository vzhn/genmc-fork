#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>

/*
 * The MCS queue in the slow path of Linux's qspinlock
 * (queued_spin_lock_slowpath()), reduced to the queue: the pending bit and
 * the locked byte are left out, so the thread at the head of the queue
 * owns the lock. A thread initializes its node (locked = 0, next = NULL),
 * publishes it with xchg_tail(), whose release order covers only its own
 * write, links itself into its predecessor's next and waits until its
 * locked becomes 1. The owner passes the lock on by setting the next
 * node's locked to 1; with no successor in sight it tries to clear the
 * tail, and if that fails it waits for the successor's link.
 *
 * The kernel relies on address dependencies in two places: from the old
 * tail returned by xchg_tail() to the store into prev->next, and from the
 * relaxed load of the successor pointer to the hand-off store. C11 has no
 * dependency ordering, so this model gives the exchange acquire order as
 * well and reads the successor pointer with acquire order.
 *
 * NTHREADS threads take the lock once each; the critical sections never
 * overlap and nobody waits forever.
 *
 * BUG_PLAIN_LINK: the link into the predecessor is WRITE_ONCE(), a relaxed
 * store, as in Linux 4.5 to 4.15. Nothing orders the node's
 * initialization before it: the read part of xchg_tail() is not ordered
 * after those stores, and its release covers only its write part. The
 * predecessor can follow the link and set locked = 1 before the
 * initializing locked = 0 lands, which then overwrites it, and the thread
 * waits forever. Commit 95bcade33a8a ("locking/qspinlock: Ensure node is
 * initialised before updating prev->next", Will Deacon, February 2018)
 * made the link smp_store_release(); VSync (Oberhauser et al., ASPLOS
 * 2021, Table 4) lists the barrier bug in 4.5 and 4.8.
 */

#ifndef NTHREADS
# define NTHREADS 2
#endif

#ifdef BUG_PLAIN_LINK
# define LINK_ORDER memory_order_relaxed
#else
# define LINK_ORDER memory_order_release
#endif

struct mcs_spinlock {
	_Atomic(struct mcs_spinlock *) next;
	atomic_int locked;
};

_Atomic(struct mcs_spinlock *) tail;
struct mcs_spinlock nodes[NTHREADS];
atomic_int in_cs;

static void queue_lock(struct mcs_spinlock *node)
{
	atomic_store_explicit(&node->locked, 0, memory_order_relaxed);
	atomic_store_explicit(&node->next, NULL, memory_order_relaxed);

	/* xchg_tail(); acquire stands in for the dependency to prev->next */
	struct mcs_spinlock *prev = atomic_exchange_explicit(&tail, node, memory_order_acq_rel);

	if (prev == NULL)
		return;
	atomic_store_explicit(&prev->next, node, LINK_ORDER);
	/* arch_mcs_spin_lock_contended() */
	__VERIFIER_await(atomic_load_explicit(&node->locked, memory_order_acquire) != 0);
}

static void queue_unlock(struct mcs_spinlock *node)
{
	struct mcs_spinlock *expected = node;

	if (atomic_compare_exchange_strong_explicit(&tail, &expected, NULL,
						    memory_order_release,
						    memory_order_relaxed))
		return;
	/* a successor is linking itself in */
	__VERIFIER_await(atomic_load_explicit(&node->next, memory_order_acquire) != NULL);

	struct mcs_spinlock *next = atomic_load_explicit(&node->next, memory_order_acquire);

	/* arch_mcs_spin_unlock_contended() */
	atomic_store_explicit(&next->locked, 1, memory_order_release);
}

static void *thread(void *arg)
{
	struct mcs_spinlock *node = &nodes[(intptr_t) arg];

	queue_lock(node);
	int others = atomic_fetch_add_explicit(&in_cs, 1, memory_order_relaxed);
	assert(others == 0);
	atomic_fetch_sub_explicit(&in_cs, 1, memory_order_relaxed);
	queue_unlock(node);
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
