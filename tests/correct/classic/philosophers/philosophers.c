#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>
#include "../../../lib/sem.h"

/*
 * Dining philosophers (The Little Book of Semaphores, 4.5; Hoare, CSP; The
 * Art of Multiprocessor Programming, 1.5): N philosophers around a table,
 * one fork between each pair; a philosopher needs both neighbouring forks
 * to eat. Each fork is a semaphore. Each philosopher eats once; no fork is
 * used by two at once, and nobody waits forever.
 *
 * Without a switch: a footman lets at most N - 1 philosophers sit down
 * (4.5.2), so at least one of them can get both forks. LEFTY selects the
 * other solution (4.5.4): philosopher 0 picks up the left fork first,
 * everyone else the right one.
 *
 * BUG_DEADLOCK: everyone picks up the right fork first, then the left one.
 * If all pick up their right fork at once, they all wait forever (Deadlock
 * #5, 4.5.1).
 */

#ifndef N
#define N 3
#endif

sema_t fork_sem[N];
sema_t footman;
atomic_int fork_users[N];
atomic_int ate;

static int left(int i) { return i; }
static int right(int i) { return (i + 1) % N; }

static void get_forks(int i)
{
#if defined(BUG_DEADLOCK)
	sema_wait(&fork_sem[right(i)]);
	sema_wait(&fork_sem[left(i)]);
#elif defined(LEFTY)
	if (i == 0) {
		sema_wait(&fork_sem[left(i)]);
		sema_wait(&fork_sem[right(i)]);
	} else {
		sema_wait(&fork_sem[right(i)]);
		sema_wait(&fork_sem[left(i)]);
	}
#else
	sema_wait(&footman);
	sema_wait(&fork_sem[right(i)]);
	sema_wait(&fork_sem[left(i)]);
#endif
}

static void put_forks(int i)
{
	sema_signal(&fork_sem[right(i)]);
	sema_signal(&fork_sem[left(i)]);
#if !defined(BUG_DEADLOCK) && !defined(LEFTY)
	sema_signal(&footman);
#endif
}

static void use_fork(int f)
{
	int others = atomic_fetch_add_explicit(&fork_users[f], 1, memory_order_relaxed);

	assert(others == 0);
}

static void drop_fork(int f)
{
	atomic_fetch_sub_explicit(&fork_users[f], 1, memory_order_relaxed);
}

static void *philosopher(void *arg)
{
	int i = (intptr_t) arg;

	get_forks(i);
	use_fork(left(i));
	use_fork(right(i));
	atomic_fetch_add_explicit(&ate, 1, memory_order_relaxed);
	drop_fork(right(i));
	drop_fork(left(i));
	put_forks(i);
	return NULL;
}

int main()
{
	pthread_t t[N];

	for (int i = 0; i < N; i++)
		sema_init(&fork_sem[i], 1);
	sema_init(&footman, N - 1);
	for (intptr_t i = 0; i < N; i++)
		if (pthread_create(&t[i], NULL, philosopher, (void *) i))
			abort();
	for (int i = 0; i < N; i++)
		if (pthread_join(t[i], NULL))
			abort();
	assert(ate == N);
	return 0;
}
