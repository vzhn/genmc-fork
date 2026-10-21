/* A load whose path to the assume goes through an effect-free loop: the
 * load annotation DFS meets a cycle (sentinel: true) */
#include <pthread.h>
#include <stdatomic.h>
#include <genmc.h>
atomic_int x;
void *t1(void *u) {
	int r = atomic_load(&x);
	for (int i = 0; i < 2; i++)
		;
	__VERIFIER_assume(r == 1);
	return NULL;
}
void *t2(void *u) { atomic_store(&x, 1); atomic_store(&x, 2); return NULL; }
int main()
{
	pthread_t a, b;

	pthread_create(&a, NULL, t1, NULL);
	pthread_create(&b, NULL, t2, NULL);
	pthread_join(a, NULL);
	pthread_join(b, NULL);
	return 0;
}
