#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>
#include "../../../lib/lock.h"

/*
 * glibc's reader-writer lock (nptl/pthread_rwlock_common.c, the algorithm
 * of glibc 2.25 and later), reduced to the paths that meet in bug 23844:
 * rdlock and rdunlock, trywrlock and the unlock that follows it, with the
 * default reader preference, no timeouts and no blocked writers.
 *
 * The lock word __readers holds a phase bit (write phase or read phase),
 * a WRLOCKED bit for the primary writer, and the reader count. Readers
 * that find a write phase owned by a writer block on __wrphase_futex,
 * which is 1 during a write phase and 0 during a read phase; a reader
 * about to sleep adds FUTEX_USED to it, and whoever ends the write phase
 * swaps it to 0 and wakes the futex if FUTEX_USED was set.
 *
 * futex_wait() checks the word and goes to sleep in one step under the
 * futex bucket lock, and only futex_wake() brings the sleeper back: a
 * later change of the word alone does not.
 *
 * The lock starts idle in a write phase (a writer has come and gone with
 * no readers waiting), so __wrphase_futex is 1. A reader takes and drops
 * a read lock while a writer tries to take the lock with trywrlock. The
 * reader always gets in.
 *
 * BUG_TRYWRLOCK_STORE: trywrlock always stores 1 to __wrphase_futex after
 * it gets the lock, as before commit 5fc9ed4c4058 ("nptl: Fix
 * pthread_rwlock_try*lock stalls (Bug 23844)", 2019). When the lock was
 * already in a write phase, a reader may have set FUTEX_USED and gone to
 * sleep in between; the store drops the flag, the writer's unlock sees no
 * one to wake, and the reader sleeps forever. The fix stores 1 only when
 * trywrlock started the write phase itself. Fails under SC too: this is a
 * lost flag, not a memory-ordering bug.
 */

#define WRPHASE		1u
#define WRLOCKED	2u
#define READER_SHIFT	3
#define ONE_READER	(1u << READER_SHIFT)
#define FUTEX_USED	2u

static unsigned nreaders(unsigned r)
{
	return r >> READER_SHIFT;
}

atomic_uint readers;
atomic_uint wrphase_futex;

/* the kernel side of __wrphase_futex */
lock_t futex_bucket;
atomic_int futex_sleepers;
atomic_int futex_wakeups;

static void futex_wait(atomic_uint *word, unsigned val)
{
	lock_acquire(&futex_bucket);
	if (atomic_load_explicit(word, memory_order_relaxed) != val) {
		lock_release(&futex_bucket);
		return;
	}
	int seen = atomic_load_explicit(&futex_wakeups, memory_order_relaxed);
	atomic_fetch_add_explicit(&futex_sleepers, 1, memory_order_relaxed);
	lock_release(&futex_bucket);
	__VERIFIER_await(atomic_load_explicit(&futex_wakeups, memory_order_acquire) != seen);
}

static void futex_wake(atomic_uint *word)
{
	lock_acquire(&futex_bucket);
	if (atomic_load_explicit(&futex_sleepers, memory_order_relaxed) != 0) {
		atomic_store_explicit(&futex_sleepers, 0, memory_order_relaxed);
		atomic_fetch_add_explicit(&futex_wakeups, 1, memory_order_release);
	}
	lock_release(&futex_bucket);
}

/* End the write phase on behalf of the readers: wake them if they sleep. */
static void hand_over_to_readers(void)
{
	if (atomic_exchange_explicit(&wrphase_futex, 0, memory_order_relaxed) & FUTEX_USED)
		futex_wake(&wrphase_futex);
}

static void rdlock(void)
{
	unsigned r = atomic_fetch_add_explicit(&readers, ONE_READER, memory_order_acquire) + ONE_READER;

	if (!(r & WRPHASE))
		return;
	/* a write phase with no primary writer: start a read phase */
	while ((r & WRPHASE) && !(r & WRLOCKED)) {
		if (atomic_compare_exchange_strong_explicit(&readers, &r, r ^ WRPHASE,
							    memory_order_acquire,
							    memory_order_relaxed)) {
			hand_over_to_readers();
			return;
		}
	}
	/* a writer owns the write phase: wait for it to hand over */
	for (;;) {
		unsigned wpf;

		while (((wpf = atomic_load_explicit(&wrphase_futex, memory_order_relaxed))
			| FUTEX_USED) == (1u | FUTEX_USED)) {
			if (!(wpf & FUTEX_USED) &&
			    !atomic_compare_exchange_strong_explicit(&wrphase_futex, &wpf,
								     wpf | FUTEX_USED,
								     memory_order_relaxed,
								     memory_order_relaxed))
				continue;
			futex_wait(&wrphase_futex, 1u | FUTEX_USED);
		}
		/* __wrphase_futex is 0: done once __readers shows the read phase */
		if (!(atomic_load_explicit(&readers, memory_order_acquire) & WRPHASE))
			return;
	}
}

static void rdunlock(void)
{
	unsigned r = atomic_load_explicit(&readers, memory_order_relaxed);
	unsigned rnew;

	do {
		rnew = r - ONE_READER;
		/* the last reader hands over to a waiting primary writer */
		if (nreaders(rnew) == 0 && (rnew & WRLOCKED))
			rnew |= WRPHASE;
	} while (!atomic_compare_exchange_strong_explicit(&readers, &r, rnew,
							  memory_order_release,
							  memory_order_relaxed));
	if (rnew & WRPHASE) {
		atomic_thread_fence(memory_order_acquire);
		if (atomic_exchange_explicit(&wrphase_futex, 1, memory_order_relaxed) & FUTEX_USED)
			futex_wake(&wrphase_futex);
	}
}

static bool trywrlock(void)
{
	unsigned r = atomic_load_explicit(&readers, memory_order_relaxed);

	while (!(r & WRLOCKED) && nreaders(r) == 0) {
		if (atomic_compare_exchange_strong_explicit(&readers, &r,
							    r | WRPHASE | WRLOCKED,
							    memory_order_acquire,
							    memory_order_relaxed)) {
#ifndef BUG_TRYWRLOCK_STORE
			if (!(r & WRPHASE))
#endif
				atomic_store_explicit(&wrphase_futex, 1, memory_order_relaxed);
			return true;
		}
	}
	return false;	/* EBUSY */
}

static void wrunlock(void)
{
	unsigned r = atomic_load_explicit(&readers, memory_order_relaxed);
	unsigned rnew;

	do {
		/* with readers waiting, hand the lock over to them */
		rnew = (r ^ WRLOCKED) ^ (nreaders(r) == 0 ? 0 : WRPHASE);
	} while (!atomic_compare_exchange_strong_explicit(&readers, &r, rnew,
							  memory_order_release,
							  memory_order_relaxed));
	if (nreaders(r) != 0)
		hand_over_to_readers();
}

static void *reader(void *arg)
{
	rdlock();
	rdunlock();
	return NULL;
}

static void *writer(void *arg)
{
	if (trywrlock())
		wrunlock();
	return NULL;
}

int main()
{
	pthread_t r, w;

	/* idle, in a write phase */
	atomic_init(&readers, WRPHASE);
	atomic_init(&wrphase_futex, 1);
	if (pthread_create(&r, NULL, reader, NULL) ||
	    pthread_create(&w, NULL, writer, NULL))
		abort();
	if (pthread_join(r, NULL) || pthread_join(w, NULL))
		abort();
	return 0;
}
