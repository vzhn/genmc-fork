#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>

/*
 * An MCS lock ported from x86 to Arm in a commercial OS, as the VSync paper
 * describes it (Oberhauser et al., ASPLOS 2021, Sec. 6.2; technical report
 * arXiv:2102.06590, Sec. 3.2 and Fig. 18). The original uses volatile
 * accesses, smp_wmb()/smp_mb() and GCC's __sync builtins; here volatile
 * accesses are relaxed atomics, both barriers are SC fences (as in the
 * paper), __sync_lock_test_and_set() is an acquire exchange and
 * __sync_val_compare_and_swap() an SC compare-and-swap.
 *
 * Acquire: clear next and set spin in one's own node, fence, swap the node
 * into the tail; behind a predecessor, link into the predecessor's next,
 * fence and spin until spin is cleared. Release: with no successor in
 * sight try to swing the tail back to null, else wait for the link; then
 * fence and clear the successor's spin.
 *
 * NTHREADS threads each increment a plain counter under the lock; the
 * increments never race and none is lost.
 *
 * BUG_NO_ACQUIRE: the acquire path ends with the relaxed spin loop, the
 * way the port shipped. The releaser's fence comes before its relaxed
 * clear of spin, but nothing after the waiter's relaxed reads pairs with
 * it, so the waiter's critical section is not ordered after the
 * releaser's: the two increments race (and one can be lost). The fix is
 * an acquire barrier after the loop.
 */

#ifndef NTHREADS
# define NTHREADS 2
#endif

struct mcs_node {
	_Atomic(struct mcs_node *) next;
	atomic_int spin;
};

_Atomic(struct mcs_node *) tail;
struct mcs_node nodes[NTHREADS];
int counter;

static void mcslock_acquire(struct mcs_node *me)
{
	atomic_store_explicit(&me->next, NULL, memory_order_relaxed);
	atomic_store_explicit(&me->spin, 1, memory_order_relaxed);
	atomic_thread_fence(memory_order_seq_cst);	/* smp_wmb() */

	struct mcs_node *prev = atomic_exchange_explicit(&tail, me, memory_order_acquire);

	if (prev == NULL)
		return;
	atomic_store_explicit(&prev->next, me, memory_order_relaxed);
	atomic_thread_fence(memory_order_seq_cst);	/* smp_mb() */
	__VERIFIER_await(atomic_load_explicit(&me->spin, memory_order_relaxed) == 0);
#ifndef BUG_NO_ACQUIRE
	atomic_thread_fence(memory_order_acquire);
#endif
}

static void mcslock_release(struct mcs_node *me)
{
	if (atomic_load_explicit(&me->next, memory_order_relaxed) == NULL) {
		struct mcs_node *expected = me;

		if (atomic_compare_exchange_strong_explicit(&tail, &expected, NULL,
							    memory_order_seq_cst,
							    memory_order_seq_cst))
			return;
		__VERIFIER_await(atomic_load_explicit(&me->next, memory_order_relaxed) != NULL);
	}
	atomic_thread_fence(memory_order_seq_cst);	/* smp_mb() */

	struct mcs_node *next = atomic_load_explicit(&me->next, memory_order_relaxed);

	atomic_store_explicit(&next->spin, 0, memory_order_relaxed);
}

static void *thread(void *arg)
{
	struct mcs_node *me = &nodes[(intptr_t) arg];

	mcslock_acquire(me);
	counter++;
	mcslock_release(me);
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
	assert(counter == NTHREADS);
	return 0;
}
