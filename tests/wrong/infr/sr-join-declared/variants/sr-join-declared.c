#include <genmc.h>
#include <stdatomic.h>

/* Threads declared symmetric are joined out of spawn order: symmetry
 * reduction does not apply (with declared symmetry, GenMC reports it
 * instead of starting over without symmetry reduction) */

atomic_int x;

void *thread(void *unused)
{
	atomic_fetch_add_explicit(&x, 1, memory_order_relaxed);
	return NULL;
}

int main()
{
	__VERIFIER_thread_t a = __VERIFIER_spawn(thread, NULL);
	__VERIFIER_thread_t b = __VERIFIER_spawn_symmetric(thread, NULL, a);

	__VERIFIER_join(b);
	__VERIFIER_join(a);
	return 0;
}
