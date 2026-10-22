#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>

/*
 * Symmetric threads with a thread-local each (issue #90): their copies are
 * different locations, which a worker publishes and the next one writes
 * through (before the owner ends: a thread's copy ends with it). Symmetry
 * reduction explores a sixth of the executions (-disable-sr: all).
 */

#define N 3

static __thread atomic_int mine;
atomic_int *_Atomic slots[N];
atomic_int next;
atomic_int done;

void *worker(void *unused)
{
	atomic_store_explicit(&mine, 1, memory_order_relaxed);
	int i = atomic_fetch_add_explicit(&next, 1, memory_order_relaxed);
	atomic_store_explicit(&slots[i], &mine, memory_order_release);

	/* Bump the copy of the worker before, if it has published it */
	atomic_int *prev = i > 0 ? atomic_load_explicit(&slots[i - 1], memory_order_acquire) : NULL;
	if (prev) {
		assert(prev != &mine);
		atomic_fetch_add_explicit(prev, 1, memory_order_relaxed);
	}

	/* Keep our copy until all workers are done with the others' */
	atomic_fetch_add_explicit(&done, 1, memory_order_acq_rel);
	while (atomic_load_explicit(&done, memory_order_acquire) < N)
		;

	/* Only the worker after can have bumped ours */
	int v = atomic_load_explicit(&mine, memory_order_relaxed);
	assert(v == 1 || (v == 2 && i < N - 1));
	return NULL;
}

int main()
{
	pthread_t t[N];

	for (int i = 0; i < N; i++)
		pthread_create(&t[i], NULL, worker, NULL);
	for (int i = 0; i < N; i++)
		pthread_join(t[i], NULL);
	return 0;
}
