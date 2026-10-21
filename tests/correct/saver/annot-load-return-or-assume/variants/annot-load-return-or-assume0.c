/* A load whose path to the assume may return first (a dead end for the
 * annotation: sentinel true, so the value is kept) or reach the assume */
#include <pthread.h>
#include <stdatomic.h>
#include <genmc.h>
atomic_int x;
void *t1(void *u) {
	int r = atomic_load(&x);
	if (r > 5)
		return NULL;
	__VERIFIER_assume(r == 1);
	return NULL;
}
void *t2(void *u) { atomic_store(&x, 1); atomic_store(&x, 7); atomic_store(&x, 2); return NULL; }
int main()
{
	pthread_t a, b;

	pthread_create(&a, NULL, t1, NULL);
	pthread_create(&b, NULL, t2, NULL);
	pthread_join(a, NULL);
	pthread_join(b, NULL);
	return 0;
}
