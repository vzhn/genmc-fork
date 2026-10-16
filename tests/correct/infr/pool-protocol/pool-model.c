#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <assert.h>
#include <genmc.h>

/*
 * A model of GenMC's own thread pool (genmc/Support/ThreadPool.*), included
 * by the pool-* tests. Workers pop tasks from a queue (queue mutex), sleep on
 * a condition variable under the state mutex when there is nothing to do, and
 * the last task to finish wakes everyone. A task may submit children, as a
 * revisit does, and with HALT one task finds an error and halts the pool.
 *
 * The condition variables are generation counters: a wait reads the counter
 * under the state mutex, releases the mutex and waits until it changes; a
 * notify bumps it. A notify that comes before the waiter reads the counter
 * is missed, as with a real condition variable.
 *
 * One switch per fix (FIXED selects all three); without it, the original:
 * - FIX_WAKEUP: popTask() re-checks the queue under the state mutex before
 *   waiting (the original can sleep while a task is queued: lost wakeup);
 * - FIX_HALT: popTask() checks the halt and pops under the state mutex (the
 *   original only checks it when the queue is empty: tasks taken after a halt);
 * - FIX_WAIT: waitForTasks() waits on its own condition variable (the
 *   original busy-waits).
 *
 * Limits: a notify wakes every waiter (notify_one is modelled as
 * notify_all), so the model cannot tell which waiter a notify_one reaches.
 * It checks that the main thread is woken with FIX_WAIT (-check-liveness),
 * but no property fails for the busy wait itself, a waste of CPU rather than
 * an error. The halt property is "a worker that saw the halt starts no task",
 * weaker than "no task starts after halt() returns", which FIX_HALT also
 * gives the real pool by popping under the state mutex.
 *
 * Properties: a worker never goes to sleep while a task is queued; a worker
 * that saw the halt starts no other task; the main thread is woken.
 */

#ifdef FIXED
#define FIX_WAKEUP
#define FIX_HALT
#define FIX_WAIT
#endif

#define NWORKERS 2
#define DEPTH 1		/* the seed task submits children (depth 0) */
#ifndef CHILDREN
#ifdef HALT
#define CHILDREN 2	/* the first child to run halts the pool */
#else
#define CHILDREN 1
#endif
#endif

pthread_mutex_t qm;	/* queue */
pthread_mutex_t sm;	/* pool state */
int queue[CHILDREN + 1];
int qlen;
atomic_uint remaining;
atomic_bool halt;
atomic_uint state_cv;	/* generation counters */
atomic_uint done_cv;
atomic_int executed;
atomic_int zeros;	/* depth-0 tasks run */

static void cv_wait(atomic_uint *cv)	/* the caller holds sm */
{
	unsigned g = atomic_load(cv);

	pthread_mutex_unlock(&sm);
	__VERIFIER_await(atomic_load(cv) != g);
	pthread_mutex_lock(&sm);
}

static void cv_notify(atomic_uint *cv) { atomic_fetch_add(cv, 1); }

static bool queue_empty(void)
{
	pthread_mutex_lock(&qm);
	bool e = qlen == 0;
	pthread_mutex_unlock(&qm);
	return e;
}

static void submit(int depth)
{
	pthread_mutex_lock(&sm);
	atomic_fetch_add(&remaining, 1);
	pthread_mutex_lock(&qm);
	queue[qlen++] = depth;
	pthread_mutex_unlock(&qm);
	cv_notify(&state_cv);	/* notify_one */
	pthread_mutex_unlock(&sm);
}

static void pool_halt(void)
{
	pthread_mutex_lock(&sm);
	atomic_store(&halt, true);
	cv_notify(&state_cv);	/* notify_all */
	cv_notify(&done_cv);
	pthread_mutex_unlock(&sm);
}

static int try_pop(void)
{
	int t = -1;

	pthread_mutex_lock(&qm);
	if (qlen > 0)
		t = queue[--qlen];
	pthread_mutex_unlock(&qm);
	return t;
}

static int pop_task(void)
{
	for (;;) {
#ifdef FIX_HALT
		pthread_mutex_lock(&sm);
		if (atomic_load(&halt)) {
			pthread_mutex_unlock(&sm);
			return -1;
		}
		int t = try_pop();
		pthread_mutex_unlock(&sm);
#else
		int t = try_pop();
#endif

		if (t >= 0)
			return t;
		pthread_mutex_lock(&sm);
		if (atomic_load(&halt) || atomic_load(&remaining) == 0) {
			pthread_mutex_unlock(&sm);
			return -1;
		}
#ifdef FIX_WAKEUP
		/* submit() pushes under sm: re-check the queue under it */
		if (!queue_empty()) {
			pthread_mutex_unlock(&sm);
			continue;
		}
#endif
		assert(queue_empty());	/* never sleep while a task is queued */
		cv_wait(&state_cv);
		pthread_mutex_unlock(&sm);
	}
}

/* A task: stops at once if the pool halted (as an exploration does) */
static bool run_task(int t)
{
	if (atomic_load(&halt))
		return false;
	atomic_fetch_add(&executed, 1);
#ifdef HALT
	if (t == 0 && atomic_fetch_add(&zeros, 1) == 0) {
		pool_halt();	/* this task found an error */
		return true;
	}
#endif
	for (int i = 0; t > 0 && i < CHILDREN; i++)
		submit(t - 1);
	return true;
}

static void *worker(void *unused)
{
	bool saw_halt = false;

	for (;;) {
		int t = pop_task();

		if (t < 0)
			break;
		assert(!saw_halt);	/* a worker that saw the halt starts no task */
		if (!run_task(t))
			saw_halt = true;
		pthread_mutex_lock(&sm);
		if (atomic_fetch_sub(&remaining, 1) == 1) {
			cv_notify(&state_cv);	/* notify_all */
			cv_notify(&done_cv);
			pthread_mutex_unlock(&sm);
			break;
		}
		pthread_mutex_unlock(&sm);
	}
	return NULL;
}

static void wait_for_tasks(void)
{
#ifdef FIX_WAIT
	pthread_mutex_lock(&sm);
	while (!atomic_load(&halt) && atomic_load(&remaining) > 0)
		cv_wait(&done_cv);
	pthread_mutex_unlock(&sm);
#else
	while (!atomic_load(&halt) && atomic_load(&remaining) > 0)
		;	/* busy-wait */
#endif
}

int main()
{
	pthread_t w[NWORKERS];

	pthread_mutex_init(&qm, NULL);
	pthread_mutex_init(&sm, NULL);
	submit(DEPTH);
	for (int i = 0; i < NWORKERS; i++)
		pthread_create(&w[i], NULL, worker, NULL);
	wait_for_tasks();
	assert(atomic_load(&halt) || atomic_load(&remaining) == 0);
	for (int i = 0; i < NWORKERS; i++)
		pthread_join(w[i], NULL);
#ifndef HALT
	assert(atomic_load(&executed) == CHILDREN + 1);
#endif
	return 0;
}
