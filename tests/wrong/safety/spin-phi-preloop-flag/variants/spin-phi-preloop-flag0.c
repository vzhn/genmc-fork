#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>

/*
 * A header PHI that holds a CAS's success flag is false whenever an iteration
 * repeats, except that here its first value comes from a CAS before the loop,
 * which succeeds. The first iteration therefore differs from the second one:
 * it only resets the flag. The spin-assume transformation pruned it, so the
 * second iteration, whose CAS succeeds and reaches the assertion, was never
 * explored.
 */

atomic_int x;

static void *thread(void *unused)
{
	int zero = 0;
	bool ok = atomic_compare_exchange_strong(&x, &zero, 1);
	for (;;) {
		if (ok) {
			ok = false;
			continue;
		}
		int one = 1;
		ok = atomic_compare_exchange_strong(&x, &one, 2);
		if (ok)
			break;
	}
	assert(0);
	return NULL;
}

int main()
{
	pthread_t a;
	pthread_create(&a, NULL, thread, NULL);
	return 0;
}
