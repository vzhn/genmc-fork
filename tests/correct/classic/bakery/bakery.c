#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>

/*
 * Lamport's bakery lock (The Art of Multiprocessor Programming, Fig. 2.9),
 * in the form of Lamport's original paper: a thread announces that it is
 * choosing, takes a number one larger than any it sees, and waits for every
 * thread that is choosing, and for every thread with a smaller
 * (number, thread id). N threads each take the lock once; seq_cst accesses.
 *
 * One switch per bug:
 * - BUG_NO_CHOOSING: no "choosing" flag. A thread can read another one's
 *   number while that one is still computing it, see 0 and enter; the other
 *   one then takes the same number, wins the tie and enters too;
 * - BUG_NO_TIEBREAK: numbers are compared without the thread id. Two
 *   threads that take the same number both enter.
 */

#ifndef N
#define N 2
#endif

atomic_bool choosing[N];
atomic_int number[N];
atomic_int in_cs;	/* threads in the critical section */

/* Must thread j go first? (j has a number and (number[j], j) < (mine, i)) */
static bool goes_first(int j, int i, int mine)
{
	int n = atomic_load(&number[j]);

#ifdef BUG_NO_TIEBREAK
	return n != 0 && n < mine;
#else
	return n != 0 && (n < mine || (n == mine && j < i));
#endif
}

static void lock(int i)
{
	int max = 0;

#ifndef BUG_NO_CHOOSING
	atomic_store(&choosing[i], true);
#endif
	for (int j = 0; j < N; j++) {
		int n = atomic_load(&number[j]);
		if (n > max)
			max = n;
	}
	atomic_store(&number[i], max + 1);
#ifndef BUG_NO_CHOOSING
	atomic_store(&choosing[i], false);
#endif

	for (int j = 0; j < N; j++) {
		if (j == i)
			continue;
#ifndef BUG_NO_CHOOSING
		__VERIFIER_await(!atomic_load(&choosing[j]));
#endif
		__VERIFIER_await(!goes_first(j, i, max + 1));
	}
}

static void unlock(int i)
{
	atomic_store(&number[i], 0);
}

static void *thread(void *arg)
{
	int me = (intptr_t) arg;

	lock(me);
	int others = atomic_fetch_add_explicit(&in_cs, 1, memory_order_relaxed);
	assert(others == 0);
	atomic_fetch_sub_explicit(&in_cs, 1, memory_order_relaxed);
	unlock(me);
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
	return 0;
}
