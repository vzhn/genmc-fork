/* -exhaustive under -imm: a thread that had only its begin kept the create
 * of the thread, without the join before it in main (see
 * tests/correct/infr/imm-exhaustive-kept-begin). The views of the kept
 * create, and of t3, lacked the join, and revisits were taken whose graphs
 * are not consistent: here 1 of them (on tests/correct/saver/spin-fmutex,
 * 15003). The execution count is the same either way; ctest
 * "exhaustive-kept-create" checks that no revisit is dropped as
 * inconsistent (scripts/check-exhaustive-kept-create.sh).
 */
#include <pthread.h>
#include <stdatomic.h>

atomic_int x;
atomic_int y;
atomic_int z;

void *t1(void *arg)
{
	(void) atomic_load_explicit(&y, memory_order_relaxed);
	atomic_store_explicit(&x, 1, memory_order_relaxed);
	return NULL;
}

void *t2(void *arg)
{
	(void) atomic_load_explicit(&z, memory_order_relaxed);
	atomic_store_explicit(&y, 1, memory_order_relaxed);
	return NULL;
}

void *t3(void *arg)
{
	(void) atomic_load_explicit(&x, memory_order_relaxed);
	atomic_store_explicit(&z, 1, memory_order_relaxed);
	return NULL;
}

int main(void)
{
	pthread_t h1, h2, h3;
	pthread_create(&h1, NULL, t1, NULL);
	pthread_create(&h2, NULL, t2, NULL);
	pthread_join(h1, NULL);
	pthread_create(&h3, NULL, t3, NULL);
	pthread_join(h2, NULL);
	pthread_join(h3, NULL);
	return 0;
}
