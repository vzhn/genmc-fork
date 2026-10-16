#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>

/*
 * Many executions, explored with several threads (genmc.*.in), and an
 * assertion that fails in most of them: several workers find the error and
 * start replaying it for the report. A worker whose replay was cut short by
 * the halt must not carry the replay into another task (that used to abort
 * in calculateReplaySchedule). The variants repeat N = 2 and N = 3: each
 * run hits the bug with some probability, so before the fix the test failed
 * in every run. No .trace files: which worker reports, and which error graph
 * it prints, differ between parallel runs.
 */

atomic_int x, y;

void *thread_n(void *unused)
{
	atomic_fetch_add_explicit(&x, 1, memory_order_relaxed);
	atomic_fetch_add_explicit(&y, 1, memory_order_relaxed);
	return NULL;
}

int main()
{
	pthread_t t[N];

	for (int i = 0; i < N; i++)
		pthread_create(&t[i], NULL, thread_n, NULL);
	int a = atomic_load_explicit(&x, memory_order_relaxed);
	int b = atomic_load_explicit(&y, memory_order_relaxed);
	assert(a == 0 || b == 0 || a + b < 2 * N - 1);
	for (int i = 0; i < N; i++)
		pthread_join(t[i], NULL);
	return 0;
}
