#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>

/*
 * The reader-writer lock of parking_lot 0.12 (src/raw_rwlock.rs). One word
 * holds a reader count and the WRITER, WRITER_PARKED and PARKED bits.
 *
 * - A reader adds ONE_READER with an acquire CAS when no writer holds or
 *   waits for the lock, and unlocks with a release fetch_sub. The last
 *   reader out wakes a parked writer (unlock_shared_slow()), and the wake-up
 *   callback clears WRITER_PARKED.
 * - A writer sets WRITER with an acquire CAS, so no new readers get in, and
 *   then waits for the readers inside to leave (wait_for_readers()): it
 *   sets WRITER_PARKED with a CAS and parks until the last reader wakes it,
 *   then reloads the state with acquire. It unlocks with a release.
 *
 * parking_lot_core::park() rechecks the state under its bucket lock and
 * sleeps only while it still shows readers and WRITER_PARKED; the model
 * waits until that check fails. Waiting readers spin on the WRITER bit
 * instead of parking. The spinning before parking is left out.
 *
 * NREADERS readers read the data under the lock while one writer updates
 * it under the lock.
 *
 * BUG_RELAXED_PARK_CAS: the CAS that sets WRITER_PARKED was relaxed on both
 * success and failure, as before commit 3fe7233ee05f ("Fix incorrect memory
 * ordering in RwLock", parking_lot#344, 2022; found with ThreadSanitizer,
 * parking_lot#323). If the last reader leaves just before the CAS, the CAS
 * fails and its relaxed load shows no readers, so the writer leaves
 * wait_for_readers() without synchronising with the reader's release: the
 * reader's read races with the writer's update.
 */

#ifndef NREADERS
# define NREADERS 1
#endif

#ifdef BUG_RELAXED_PARK_CAS
# define PARK_CAS_ORDER memory_order_relaxed
#else
# define PARK_CAS_ORDER memory_order_acquire
#endif

#define PARKED_BIT		0x1UL
#define WRITER_PARKED_BIT	0x2UL
#define UPGRADABLE_BIT		0x4UL
#define WRITER_BIT		0x8UL
#define READERS_MASK		(~0xfUL)
#define ONE_READER		0x10UL

atomic_ulong state;
int data;

/* ---- readers ---- */

static bool try_lock_shared_fast(void)
{
	unsigned long s = atomic_load_explicit(&state, memory_order_relaxed);

	if (s & WRITER_BIT)
		return false;
	return atomic_compare_exchange_strong_explicit(&state, &s, s + ONE_READER,
						       memory_order_acquire,
						       memory_order_relaxed);
}

static void lock_shared(void)
{
	while (!try_lock_shared_fast())
		__VERIFIER_await(!(atomic_load_explicit(&state, memory_order_relaxed) &
				   WRITER_BIT));
}

static void unlock_shared(void)
{
	unsigned long s = atomic_fetch_sub_explicit(&state, ONE_READER, memory_order_release);

	if ((s & (READERS_MASK | WRITER_PARKED_BIT)) == (ONE_READER | WRITER_PARKED_BIT)) {
		/* unlock_shared_slow(): unpark_one() and its callback */
		atomic_fetch_and_explicit(&state, ~WRITER_PARKED_BIT, memory_order_relaxed);
	}
}

/* ---- the writer ---- */

static void wait_for_readers(void)
{
	unsigned long s = atomic_load_explicit(&state, memory_order_acquire);

	while (s & READERS_MASK) {
		if (!(s & WRITER_PARKED_BIT)) {
			if (!atomic_compare_exchange_strong_explicit(&state, &s,
								     s | WRITER_PARKED_BIT,
								     PARK_CAS_ORDER,
								     PARK_CAS_ORDER))
				continue;
		}
		/* park(): sleeps while validate() holds, until unparked */
		__VERIFIER_await(({
			unsigned long v = atomic_load_explicit(&state, memory_order_relaxed);
			!((v & READERS_MASK) && (v & WRITER_PARKED_BIT));
		}));
		s = atomic_load_explicit(&state, memory_order_acquire);
	}
}

static void lock_exclusive(void)
{
	unsigned long s = atomic_load_explicit(&state, memory_order_relaxed);

	/* lock_common(): set WRITER_BIT once no other writer holds it */
	for (;;) {
		if (s & WRITER_BIT) {
			__VERIFIER_await(!(atomic_load_explicit(&state, memory_order_relaxed) &
					   WRITER_BIT));
			s = atomic_load_explicit(&state, memory_order_relaxed);
			continue;
		}
		if (atomic_compare_exchange_strong_explicit(&state, &s, s | WRITER_BIT,
							    memory_order_acquire,
							    memory_order_relaxed))
			break;
	}
	wait_for_readers();
}

static void unlock_exclusive(void)
{
	atomic_fetch_and_explicit(&state, ~WRITER_BIT, memory_order_release);
}

void *reader(void *arg)
{
	lock_shared();
	int d = data;
	assert(d == 0 || d == 1);
	unlock_shared();
	return NULL;
}

void *writer(void *arg)
{
	lock_exclusive();
	data = 1;
	unlock_exclusive();
	return NULL;
}

int main()
{
	pthread_t r[NREADERS], w;

	for (int i = 0; i < NREADERS; i++)
		pthread_create(&r[i], NULL, reader, NULL);
	pthread_create(&w, NULL, writer, NULL);

	return 0;
}
