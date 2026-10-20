#include <stdatomic.h>
#include <genmc.h>

/* The handles are stored into a global array the workers read: the second
 * worker may see the first worker's handle, the first cannot */

#define N 2

atomic_int t[N];
atomic_int x;

void *worker(void *unused)
{
	if (atomic_load_explicit(&t[0], memory_order_relaxed) != 0)
		atomic_fetch_add_explicit(&x, 1, memory_order_relaxed);
	else
		atomic_store_explicit(&x, 42, memory_order_relaxed);
	return NULL;
}

int main()
{
	for (int i = 0; i < N; i++)
		atomic_store_explicit(&t[i], __VERIFIER_spawn(worker, NULL), memory_order_relaxed);
	for (int i = 0; i < N; i++)
		__VERIFIER_join(t[i]);
	return 0;
}
