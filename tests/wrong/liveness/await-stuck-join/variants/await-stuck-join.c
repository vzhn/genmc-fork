#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <genmc.h>

/*
 * A worker pool whose last task wakes nobody: a worker that went to sleep
 * waits forever, and main blocks joining it. The liveness violation is found
 * when the execution ends; before printing it GenMC replays the execution up
 * to the newest label, here main's last read before its join, a non-atomic
 * access that the replay schedule used to drop. scripts/check-liveness-report.sh
 * also runs this test with default flags (estimation on), where the replay
 * that missed its target used to abort the next execution.
 */

#define NWORKERS 2

pthread_mutex_t qm;	/* queue */
pthread_mutex_t sm;	/* pool state */
int queue[2];
int qlen;
atomic_uint remaining;
atomic_uint state_cv;	/* generation counter */

static void cv_wait(void)	/* the caller holds sm */
{
	unsigned g = atomic_load(&state_cv);

	pthread_mutex_unlock(&sm);
	__VERIFIER_await(atomic_load(&state_cv) != g);
	pthread_mutex_lock(&sm);
}

static void submit(int depth)
{
	pthread_mutex_lock(&sm);
	atomic_fetch_add(&remaining, 1);
	pthread_mutex_lock(&qm);
	queue[qlen++] = depth;
	pthread_mutex_unlock(&qm);
	atomic_fetch_add(&state_cv, 1);
	pthread_mutex_unlock(&sm);
}

static int pop_task(void)
{
	for (;;) {
		int t = -1;

		pthread_mutex_lock(&sm);
		pthread_mutex_lock(&qm);
		if (qlen > 0)
			t = queue[--qlen];
		pthread_mutex_unlock(&qm);
		if (t >= 0 || atomic_load(&remaining) == 0) {
			pthread_mutex_unlock(&sm);
			return t;
		}
		cv_wait();
		pthread_mutex_unlock(&sm);
	}
}

static void *worker(void *unused)
{
	for (;;) {
		int t = pop_task();

		if (t < 0)
			break;
		if (t > 0)
			submit(t - 1);
		pthread_mutex_lock(&sm);
		if (atomic_fetch_sub(&remaining, 1) == 1) {
			/* the bug: the last task wakes nobody */
			pthread_mutex_unlock(&sm);
			break;
		}
		pthread_mutex_unlock(&sm);
	}
	return NULL;
}

int main()
{
	pthread_t w[NWORKERS];

	pthread_mutex_init(&qm, NULL);
	pthread_mutex_init(&sm, NULL);
	submit(1);
	for (int i = 0; i < NWORKERS; i++)
		pthread_create(&w[i], NULL, worker, NULL);
	for (int i = 0; i < NWORKERS; i++)
		pthread_join(w[i], NULL);
	return 0;
}
