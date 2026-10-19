#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>

/*
 * The node-reclaim bit lock (mm/vmscan.c node_reclaim()). One bit,
 * PGDAT_RECLAIM_LOCKED, keeps node reclaim of a pglist_data exclusive: it
 * is taken with test_and_set_bit() as a trylock and released after
 * __node_reclaim(). Kernel primitives map to C11: test_and_set_bit() is a
 * relaxed fetch_or between two seq_cst fences (fully ordered), clear_bit()
 * a relaxed fetch_and and clear_bit_unlock() a release fetch_and.
 *
 * NTHREADS threads try to reclaim the node; each one that gets the bit
 * updates plain per-node state. The updates never race.
 *
 * BUG_CLEAR_BIT: the bit is released with clear_bit(), which has no
 * ordering, as before commit c0ebbb3841e0 ("mm: add missing release
 * barrier on PGDAT_RECLAIM_LOCKED unlock", 2025). The next thread to take
 * the bit is not ordered after the previous reclaim, and its accesses
 * race with that reclaim's. Passes under SC.
 */

#ifndef NTHREADS
# define NTHREADS 2
#endif

#define PGDAT_RECLAIM_LOCKED 1UL

atomic_ulong pgdat_flags;
int reclaim_state;	/* stands in for what __node_reclaim() touches */

static bool test_and_set_bit(atomic_ulong *word, unsigned long mask)
{
	atomic_thread_fence(memory_order_seq_cst);
	unsigned long old = atomic_fetch_or_explicit(word, mask, memory_order_relaxed);
	atomic_thread_fence(memory_order_seq_cst);
	return old & mask;
}

static void *node_reclaim(void *arg)
{
	if (test_and_set_bit(&pgdat_flags, PGDAT_RECLAIM_LOCKED))
		return NULL;	/* NODE_RECLAIM_NOSCAN */
	/* __node_reclaim() */
	reclaim_state++;
#ifdef BUG_CLEAR_BIT
	atomic_fetch_and_explicit(&pgdat_flags, ~PGDAT_RECLAIM_LOCKED, memory_order_relaxed);
#else
	atomic_fetch_and_explicit(&pgdat_flags, ~PGDAT_RECLAIM_LOCKED, memory_order_release);
#endif
	return NULL;
}

int main()
{
	pthread_t t[NTHREADS];

	for (int i = 0; i < NTHREADS; i++)
		if (pthread_create(&t[i], NULL, node_reclaim, NULL))
			abort();
	for (int i = 0; i < NTHREADS; i++)
		if (pthread_join(t[i], NULL))
			abort();
	return 0;
}
