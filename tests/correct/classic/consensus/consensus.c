#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>

/*
 * Consensus (The Art of Multiprocessor Programming, ch. 5): N threads each
 * propose a value and decide one. All decide the same value (agreement), and
 * it is one of the proposed values (validity).
 *
 * Without a switch: the generic protocol of Fig. 5.6 with compareAndSet()
 * (Fig. 5.16). A thread writes its proposal, then tries to install its id as
 * the winner; it decides the winner's proposal.
 *
 * One switch per bug:
 * - BUG_RW: the winner is installed with a read and a write instead of a
 *   CAS. Read/write registers cannot solve consensus (Theorem 5.2.1): two
 *   threads both see no winner, both install themselves and decide
 *   differently;
 * - BUG_RELAXED: the CAS is relaxed, so a losing thread reads the winner's
 *   proposal (a plain variable) without synchronizing with it: a data race.
 */

#ifndef N
#define N 3
#endif

#define NONE -1

#ifdef BUG_RELAXED
#define MO_CAS memory_order_relaxed
#define MO_FAIL memory_order_relaxed
#else
#define MO_CAS memory_order_acq_rel
#define MO_FAIL memory_order_acquire
#endif

int proposed[N];
atomic_int winner = NONE;
int decided[N];

static int decide(int me, int value)
{
	proposed[me] = value;
#ifdef BUG_RW
	if (atomic_load(&winner) == NONE)
		atomic_store(&winner, me);
	return proposed[atomic_load(&winner)];
#else
	int w = NONE;

	if (atomic_compare_exchange_strong_explicit(&winner, &w, me, MO_CAS, MO_FAIL))
		return value;
	return proposed[w];
#endif
}

static void *thread(void *arg)
{
	int me = (intptr_t) arg;

	decided[me] = decide(me, 10 + me);
	return NULL;
}

int main()
{
	pthread_t t[N];

	for (intptr_t i = 0; i < N; i++)
		if (pthread_create(&t[i], NULL, thread, (void *) i))
			abort();
	for (int i = 0; i < N; i++)
		if (pthread_join(t[i], NULL))
			abort();
	for (int i = 0; i < N; i++) {
		assert(decided[i] == decided[0]);		/* agreement */
		assert(decided[i] >= 10 && decided[i] < 10 + N);	/* validity */
	}
	return 0;
}
