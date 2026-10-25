#include <pthread.h>
#include <stdatomic.h>

/*
 * A read revisited although it did not read the maximal write when it was
 * added (see imm-revisit-bounded-read) is bounded by the accesses of its
 * thread after it that the revisit keeps: t0's first read cannot read a
 * write co-after the one its second read, kept, reads (CoRR). Without that
 * check, some schedules counted such incoherent executions (24 instead of
 * 20 under the arbitrary policy, seeds 2 and 3).
 */

atomic_int v0;

void *t0(void *unused)
{
	atomic_load_explicit(&v0, memory_order_relaxed);
	atomic_load_explicit(&v0, memory_order_relaxed);
	atomic_store_explicit(&v0, 1, memory_order_relaxed);
	return NULL;
}

void *t1(void *unused)
{
	atomic_store_explicit(&v0, 1, memory_order_relaxed);
	return NULL;
}

void *t2(void *unused)
{
	atomic_fetch_add_explicit(&v0, 1, memory_order_relaxed);
	return NULL;
}

int main()
{
	pthread_t t[3];

	pthread_create(&t[0], NULL, t0, NULL);
	pthread_create(&t[1], NULL, t1, NULL);
	pthread_create(&t[2], NULL, t2, NULL);
	return 0;
}
