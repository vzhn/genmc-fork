/* A store between the load and the assume: no annotation may claim the
 * assume fails without the store happening */
#include <pthread.h>
#include <stdatomic.h>
#include <genmc.h>
atomic_int x; atomic_int y;
void *t1(void *u) {
	int r = atomic_load(&x);
	atomic_store(&y, r);
	__VERIFIER_assume(r == 1);
	return NULL;
}
void *t2(void *u) { atomic_store(&x, 1); atomic_store(&x, 2); int s = atomic_load(&y); (void)s; return NULL; }
int main()
{
	pthread_t a, b;

	pthread_create(&a, NULL, t1, NULL);
	pthread_create(&b, NULL, t2, NULL);
	pthread_join(a, NULL);
	pthread_join(b, NULL);
	return 0;
}
