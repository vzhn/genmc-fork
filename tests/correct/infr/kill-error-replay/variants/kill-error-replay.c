#include <stdatomic.h>
#include <stdlib.h>
#include <genmc.h>

/*
 * A thread killed by the -unroll bound (spin-assume is disabled) with an
 * unfreed allocation: the warning starts an error replay whose target is
 * the kill event itself. The driver used to allow re-executing a kill only
 * while the error replay was still in progress, but reporting the warning
 * ends it first (INTERNAL FAILURE in handleThreadKill).
 */

atomic_int x;

int main()
{
	int *p = malloc(sizeof(int));
	while (atomic_load_explicit(&x, memory_order_relaxed) == 0)
		;
	free(p);
	return 0;
}
