#include <stdatomic.h>
#include <genmc.h>

/* The local array's address leaks through a global pointer, so the
 * workers can read the handles stored between their spawns */

#define N 2

atomic_int *_Atomic handles;
atomic_int x;

void *worker(void *unused)
{
	atomic_int *h = atomic_load_explicit(&handles, memory_order_relaxed);
	if (h && atomic_load_explicit(&h[0], memory_order_relaxed) != 0)
		atomic_fetch_add_explicit(&x, 1, memory_order_relaxed);
	else
		atomic_store_explicit(&x, 42, memory_order_relaxed);
	return NULL;
}

int main()
{
	atomic_int t[N] = {0};

	atomic_store_explicit(&handles, t, memory_order_relaxed);
	for (int i = 0; i < N; i++)
		atomic_store_explicit(&t[i], __VERIFIER_spawn(worker, NULL), memory_order_relaxed);
	for (int i = 0; i < N; i++)
		__VERIFIER_join(t[i]);
	return 0;
}
