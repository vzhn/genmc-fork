#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>

/*
 * A Chase-Lev work-stealing deque whose array is resized: a push that finds
 * it full doubles it, and a pop that leaves it less than a quarter full
 * halves it. This is the C11 deque of Lê, Pop, Cohen and Zappa Nardelli
 * (Correct and Efficient Work-Stealing for Weak Memory Models, PPoPP 2013),
 * with the shrinking and the batch steal of crossbeam-deque (Worker::pop,
 * Stealer::steal_batch). The owner (main) pushes and pops at the bottom; a
 * thief steals from the top.
 *
 * Tasks are heap-allocated and freed by whoever takes them, so a task taken
 * twice is a double free; at the end, main checks that every task was taken
 * exactly once.
 *
 * Two clients:
 * - by default (shrink): the owner pushes two tasks, pops one (the array
 *   shrinks) and pushes a third, while the thief steals up to two;
 * - GROW: the owner pushes into a one-slot array, so the second push grows
 *   it, while the thief steals one task.
 *
 * One switch per bug:
 * - BUG_STALE_ARRAY: crossbeam-deque before 0.7.4 and 0.8.1 (CVE-2021-32810).
 *   A batch steal loads the array once and keeps stealing from it. If the
 *   owner meanwhile pops a task, shrinks the array and pushes a new task at
 *   the same index, the thief takes the popped task again from the old
 *   array (a double free) and the new task is lost. The fix (crossbeam PR
 *   #726) re-checks the array before each CAS;
 * - BUG_RELAXED_ARRAY: the resize publishes the new array with a relaxed
 *   store (found in the paper's code by CDSChecker, Norris and Demsky, OOPSLA
 *   2013). A thief that loads the new array is not synchronized with its
 *   initialization and reads memory it cannot see allocated.
 */

#ifdef GROW
# define INIT_CAP 1
# define MIN_CAP 1
# define BATCH 1
#else
# define INIT_CAP 8
# define MIN_CAP 4
# define BATCH 2
#endif
#define MAX_CAP 8
#define NTASKS 3

struct array {
	long cap;
	_Atomic(int *) buf[MAX_CAP];
};

struct deque {
	atomic_long top;
	atomic_long bottom;
	_Atomic(struct array *) array;
};

struct deque q;

/* Arrays replaced by a resize, freed once the thief is done (owner only) */
struct array *retired[4];
int nretired;

int *tasks[NTASKS];
atomic_int taken[NTASKS];

static struct array *new_array(long cap)
{
	struct array *a = malloc(sizeof(*a));

	a->cap = cap;
	return a;
}

/* Copy the tasks in [t, b) into a new array of the given capacity (owner) */
static struct array *resize(struct array *a, long t, long b, long cap)
{
	struct array *na = new_array(cap);

	for (long i = t; i < b; i++)
		atomic_store_explicit(&na->buf[i % cap],
				      atomic_load_explicit(&a->buf[i % a->cap], memory_order_relaxed),
				      memory_order_relaxed);
#ifdef BUG_RELAXED_ARRAY
	atomic_store_explicit(&q.array, na, memory_order_relaxed);
#else
	atomic_store_explicit(&q.array, na, memory_order_release);
#endif
	retired[nretired++] = a;
	return na;
}

static void push(int *x)
{
	long b = atomic_load_explicit(&q.bottom, memory_order_relaxed);
	long t = atomic_load_explicit(&q.top, memory_order_acquire);
	struct array *a = atomic_load_explicit(&q.array, memory_order_relaxed);

	if (b - t >= a->cap)
		a = resize(a, t, b, 2 * a->cap);
	atomic_store_explicit(&a->buf[b % a->cap], x, memory_order_relaxed);
	atomic_thread_fence(memory_order_release);
	atomic_store_explicit(&q.bottom, b + 1, memory_order_relaxed);
}

static int *pop(void)
{
	long b = atomic_load_explicit(&q.bottom, memory_order_relaxed) - 1;
	struct array *a = atomic_load_explicit(&q.array, memory_order_relaxed);

	atomic_store_explicit(&q.bottom, b, memory_order_relaxed);
	atomic_thread_fence(memory_order_seq_cst);
	long t = atomic_load_explicit(&q.top, memory_order_relaxed);

	if (t > b) {
		/* empty */
		atomic_store_explicit(&q.bottom, b + 1, memory_order_relaxed);
		return NULL;
	}

	int *x = atomic_load_explicit(&a->buf[b % a->cap], memory_order_relaxed);

	if (t == b) {
		/* the last task: race the thief for it */
		if (!atomic_compare_exchange_strong_explicit(&q.top, &t, t + 1,
							     memory_order_seq_cst,
							     memory_order_relaxed))
			x = NULL;
		atomic_store_explicit(&q.bottom, b + 1, memory_order_relaxed);
	} else if (a->cap > MIN_CAP && b - t < a->cap / 4) {
		resize(a, t, b, a->cap / 2);
	}
	return x;
}

static void take(int *x)
{
	for (int i = 0; i < NTASKS; i++)
		if (tasks[i] == x)
			atomic_fetch_add_explicit(&taken[i], 1, memory_order_relaxed);
	free(x);
}

static void *thief(void *unused)
{
	long t = atomic_load_explicit(&q.top, memory_order_acquire);

	atomic_thread_fence(memory_order_seq_cst);
	long b = atomic_load_explicit(&q.bottom, memory_order_acquire);

	if (t >= b)
		return NULL;

	struct array *a = atomic_load_explicit(&q.array, memory_order_acquire);

	for (int i = 0; i < BATCH; i++) {
		if (i > 0) {
			atomic_thread_fence(memory_order_seq_cst);
			b = atomic_load_explicit(&q.bottom, memory_order_acquire);
			if (t >= b)
				break;
		}

		int *x = atomic_load_explicit(&a->buf[t % a->cap], memory_order_relaxed);

#ifndef BUG_STALE_ARRAY
		/* the array was replaced: this slot may be stale, retry later */
		if (atomic_load_explicit(&q.array, memory_order_acquire) != a)
			break;
#endif
		if (!atomic_compare_exchange_strong_explicit(&q.top, &t, t + 1,
							     memory_order_seq_cst,
							     memory_order_relaxed))
			break;
		take(x);
		t++;
	}
	return NULL;
}

int main()
{
	pthread_t th;
	int *x;

	for (int i = 0; i < NTASKS; i++)
		tasks[i] = malloc(sizeof(int));
	atomic_init(&q.array, new_array(INIT_CAP));

	push(tasks[0]);
#ifdef GROW
	if (pthread_create(&th, NULL, thief, NULL))
		abort();
	push(tasks[1]);
	push(tasks[2]);
#else
	push(tasks[1]);
	if (pthread_create(&th, NULL, thief, NULL))
		abort();
	if ((x = pop()))
		take(x);
	push(tasks[2]);
#endif
	while ((x = pop()))
		take(x);
	if (pthread_join(th, NULL))
		abort();

	for (int i = 0; i < nretired; i++)
		free(retired[i]);
	free(atomic_load_explicit(&q.array, memory_order_relaxed));
	for (int i = 0; i < NTASKS; i++)
		assert(taken[i] == 1);
	return 0;
}
