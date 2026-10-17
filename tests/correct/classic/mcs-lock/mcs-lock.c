#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>

/*
 * The MCS queue lock (Mellor-Crummey and Scott, Algorithms for Scalable
 * Synchronization on Shared-Memory Multiprocessors, TOCS 1991). A thread
 * appends its queue node with an exchange on tail; if there was a
 * predecessor, it marks itself locked, links itself behind the predecessor
 * and spins on its own flag. Unlock hands the lock to the successor; with
 * no successor in sight it tries to swing tail back to null, and if that
 * fails a successor is between its exchange and its link, so it waits for
 * the link first. Spins are awaits.
 *
 * NTHREADS threads take the lock once each; the critical sections never
 * overlap and nobody waits forever.
 *
 * One switch per bug, both steps the paper's code has for a reason:
 * - BUG_NO_WAIT: unlock does not wait for the successor's link when its CAS
 *   on tail fails, and hands the lock to a null successor;
 * - BUG_LOCKED_AFTER_LINK: a thread marks itself locked only after linking
 *   behind its predecessor. The predecessor can unlock in between and clear
 *   the flag, which the thread then sets: it spins forever.
 */

#ifndef NTHREADS
# define NTHREADS 2
#endif

struct qnode {
	_Atomic(struct qnode *) next;
	atomic_bool locked;
};

_Atomic(struct qnode *) tail;
struct qnode qnodes[NTHREADS];
atomic_int in_cs;

static void mcs_lock(struct qnode *me)
{
	atomic_store_explicit(&me->next, NULL, memory_order_relaxed);

	struct qnode *pred = atomic_exchange_explicit(&tail, me, memory_order_acq_rel);

	if (pred == NULL)
		return;
#ifdef BUG_LOCKED_AFTER_LINK
	atomic_store_explicit(&pred->next, me, memory_order_release);
	atomic_store_explicit(&me->locked, true, memory_order_relaxed);
#else
	atomic_store_explicit(&me->locked, true, memory_order_relaxed);
	atomic_store_explicit(&pred->next, me, memory_order_release);
#endif
	__VERIFIER_await(!atomic_load_explicit(&me->locked, memory_order_acquire));
}

static void mcs_unlock(struct qnode *me)
{
	struct qnode *next = atomic_load_explicit(&me->next, memory_order_acquire);

	if (next == NULL) {
		struct qnode *expected = me;

		if (atomic_compare_exchange_strong_explicit(&tail, &expected, NULL,
							    memory_order_release,
							    memory_order_relaxed))
			return;
#ifndef BUG_NO_WAIT
		/* a successor is linking itself in */
		__VERIFIER_await(atomic_load_explicit(&me->next, memory_order_acquire) != NULL);
		next = atomic_load_explicit(&me->next, memory_order_acquire);
#endif
	}
	atomic_store_explicit(&next->locked, false, memory_order_release);
}

static void *thread(void *arg)
{
	struct qnode *me = &qnodes[(intptr_t) arg];

	mcs_lock(me);
	int others = atomic_fetch_add_explicit(&in_cs, 1, memory_order_relaxed);
	assert(others == 0);
	atomic_fetch_sub_explicit(&in_cs, 1, memory_order_relaxed);
	mcs_unlock(me);
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
