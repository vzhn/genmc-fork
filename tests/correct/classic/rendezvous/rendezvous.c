#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>
#include "../../../lib/sem.h"

/*
 * Rendezvous (The Little Book of Semaphores, 3.3): threads A and B each run
 * two statements, and neither runs its second statement before the other
 * has run its first. Each thread signals its own arrival, then waits for the
 * other's.
 *
 * BUG_DEADLOCK: each thread waits for the other before signalling
 * (Deadlock #1, 3.3.3): neither ever signals.
 */

sema_t a_arrived;
sema_t b_arrived;
int a1_done;
int b1_done;

static void *thread_a(void *unused)
{
	a1_done = 1;
#ifdef BUG_DEADLOCK
	sema_wait(&b_arrived);
	sema_signal(&a_arrived);
#else
	sema_signal(&a_arrived);
	sema_wait(&b_arrived);
#endif
	assert(b1_done);
	return NULL;
}

static void *thread_b(void *unused)
{
	b1_done = 1;
#ifdef BUG_DEADLOCK
	sema_wait(&a_arrived);
	sema_signal(&b_arrived);
#else
	sema_signal(&b_arrived);
	sema_wait(&a_arrived);
#endif
	assert(a1_done);
	return NULL;
}

int main()
{
	pthread_t a, b;

	if (pthread_create(&a, NULL, thread_a, NULL))
		abort();
	if (pthread_create(&b, NULL, thread_b, NULL))
		abort();
	if (pthread_join(a, NULL))
		abort();
	if (pthread_join(b, NULL))
		abort();
	return 0;
}
