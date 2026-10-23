/* Two threads free the same block: the error names the other free. The
 * checkers took the conflicting event of a double free (and of an access
 * after free) from a pointer that only other checks set: it named no event
 * here, a wrong one elsewhere, and under -exhaustive -imm one of a destroyed
 * copy of the graph (#199). See also scripts/check-free-conflict.sh.
 */
#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>

atomic_int x;
int *p;

void *t1(void *arg)
{
	atomic_store_explicit(&x, 1, memory_order_relaxed);
	free(p);
	return NULL;
}

void *t2(void *arg)
{
	(void) atomic_load_explicit(&x, memory_order_relaxed);
	free(p);
	return NULL;
}

int main(void)
{
	pthread_t a, b;

	p = malloc(sizeof(int));
	pthread_create(&a, NULL, t1, NULL);
	pthread_create(&b, NULL, t2, NULL);
	pthread_join(a, NULL);
	pthread_join(b, NULL);
	return 0;
}
