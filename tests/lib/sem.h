#ifndef CLASSIC_SEM_H
#define CLASSIC_SEM_H

/*
 * A counting semaphore, as in The Little Book of Semaphores: sema_wait()
 * waits until the value is positive and decrements it, sema_signal()
 * increments it. It is not fair: any waiting thread may get the next token.
 *
 * The value is posts - takes. A signal increments posts; a waiter claims
 * token t by moving takes from t to t + 1 once posts > t. Keeping the two
 * counters apart means GenMC does not order every wait against every
 * signal, which keeps the number of executions small. A waiting thread waits
 * in __VERIFIER_await(), so with -check-liveness a semaphore that is never
 * signalled is reported as a wait that can never finish.
 *
 * Initialize with SEMA_INIT(n) or, before the threads start, sema_init().
 */

#include <stdatomic.h>
#include <genmc.h>

typedef struct {
	atomic_int posts;
	atomic_int takes;
} sema_t;

#define SEMA_INIT(n) { (n), 0 }

static inline void sema_init(sema_t *s, int n)
{
	atomic_init(&s->posts, n);
	atomic_init(&s->takes, 0);
}

static inline void sema_wait(sema_t *s)
{
	for (;;) {
		int t = atomic_load_explicit(&s->takes, memory_order_relaxed);

		__VERIFIER_await(atomic_load_explicit(&s->posts, memory_order_acquire) > t);
		if (atomic_compare_exchange_strong_explicit(&s->takes, &t, t + 1,
							    memory_order_relaxed,
							    memory_order_relaxed))
			return;
	}
}

static inline void sema_signal(sema_t *s)
{
	atomic_fetch_add_explicit(&s->posts, 1, memory_order_release);
}

#endif /* CLASSIC_SEM_H */
