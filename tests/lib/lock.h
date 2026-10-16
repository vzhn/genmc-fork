#ifndef CLASSIC_LOCK_H
#define CLASSIC_LOCK_H

/*
 * A spinlock for the tests in correct/classic and the wrong tests built from
 * them. A thread that finds the lock taken waits in __VERIFIER_await() until
 * it is free, so with -check-liveness a lock that is never released is
 * reported as a wait that can never finish.
 */

#include <stdatomic.h>
#include <genmc.h>

typedef atomic_int lock_t;

static inline void lock_acquire(lock_t *l)
{
	for (;;) {
		int free = 0;

		if (atomic_compare_exchange_strong_explicit(l, &free, 1, memory_order_acquire,
							    memory_order_relaxed))
			return;
		__VERIFIER_await(atomic_load_explicit(l, memory_order_relaxed) == 0);
	}
}

static inline void lock_release(lock_t *l)
{
	atomic_store_explicit(l, 0, memory_order_release);
}

#endif /* CLASSIC_LOCK_H */
