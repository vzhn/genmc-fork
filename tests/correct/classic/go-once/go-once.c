#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>
#include "../../../lib/lock.h"

/*
 * Go's sync.Once (src/sync/once.go): Do(f) calls f once, and every call of
 * Do returns only after f has finished. The fast path loads done; the slow
 * path takes the mutex, checks done again, calls f and only then stores
 * done. Go's atomics are seq_cst.
 *
 * Two threads call Do(init); init sets a value, which each thread reads
 * right after Do returns.
 *
 * One switch per bug, both from the comment in once.go:
 * - BUG_CAS: "Here is an incorrect implementation of Do: if
 *   o.done.CompareAndSwap(0, 1) { f() }". The loser of the CAS returns at
 *   once, without waiting for the winner's f, and reads the value unset;
 * - BUG_EARLY_DONE: the slow path stores done before calling f ("the
 *   o.done.Store must be delayed until after f returns"). A caller on the
 *   fast path sees done and returns before f has run.
 */

atomic_uint done;
lock_t m;
atomic_int value;
atomic_int calls;

static void init(void)
{
	atomic_fetch_add_explicit(&calls, 1, memory_order_relaxed);
	atomic_store_explicit(&value, 42, memory_order_relaxed);
}

static void once_do(void (*f)(void))
{
#ifdef BUG_CAS
	unsigned int zero = 0;

	if (atomic_compare_exchange_strong(&done, &zero, 1))
		f();
#else
	if (atomic_load(&done) == 0) {
		lock_acquire(&m);
		if (atomic_load(&done) == 0) {
# ifdef BUG_EARLY_DONE
			atomic_store(&done, 1);
			f();
# else
			f();
			atomic_store(&done, 1);
# endif
		}
		lock_release(&m);
	}
#endif
}

static void *thread(void *unused)
{
	once_do(init);
	assert(atomic_load_explicit(&value, memory_order_relaxed) == 42);
	return NULL;
}

int main()
{
	pthread_t a, b;

	if (pthread_create(&a, NULL, thread, NULL))
		abort();
	if (pthread_create(&b, NULL, thread, NULL))
		abort();
	if (pthread_join(a, NULL))
		abort();
	if (pthread_join(b, NULL))
		abort();
	assert(calls == 1);
	return 0;
}
