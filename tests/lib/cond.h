#ifndef CLASSIC_COND_H
#define CLASSIC_COND_H

/*
 * A condition variable used with a lock_t (Mesa semantics, as with
 * pthread_cond_t). Waiters take tickets in arrival order; cond_signal()
 * wakes the oldest waiter not yet woken and cond_broadcast() wakes them all.
 * A notify with no waiter is lost. A woken thread takes the lock again before
 * cond_wait() returns, so another thread can take it first and change the
 * state. Not modelled: spurious wakeups, and waking waiters out of order.
 *
 * Call all three with the lock held.
 */

#include <stdatomic.h>
#include <genmc.h>
#include "lock.h"

typedef struct {
	atomic_uint waiters;	/* tickets handed out */
	atomic_uint woken;	/* tickets woken */
} cond_t;

static inline void cond_wait(cond_t *c, lock_t *l)
{
	unsigned me = atomic_load_explicit(&c->waiters, memory_order_relaxed);

	atomic_store_explicit(&c->waiters, me + 1, memory_order_relaxed);
	lock_release(l);
	__VERIFIER_await(atomic_load_explicit(&c->woken, memory_order_relaxed) > me);
	lock_acquire(l);
}

static inline void cond_signal(cond_t *c)
{
	unsigned w = atomic_load_explicit(&c->woken, memory_order_relaxed);

	if (w < atomic_load_explicit(&c->waiters, memory_order_relaxed))
		atomic_store_explicit(&c->woken, w + 1, memory_order_relaxed);
}

static inline void cond_broadcast(cond_t *c)
{
	atomic_store_explicit(&c->woken, atomic_load_explicit(&c->waiters, memory_order_relaxed),
			      memory_order_relaxed);
}

#endif /* CLASSIC_COND_H */
