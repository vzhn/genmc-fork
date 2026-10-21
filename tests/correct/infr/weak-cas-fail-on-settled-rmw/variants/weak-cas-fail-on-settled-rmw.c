#include <pthread.h>
#include <stdatomic.h>

/*
 * t3's weak CAS may read the initial value of x0 and fail spuriously while
 * t1's fetch_add reads it too; the rf is dropped (it would violate atomicity
 * if the CAS succeeded) once t1's RMW is settled by a revisit, and the
 * spurious failure with t2 reading t1's write of x0 and t1 reading t2's
 * write of x1 is lost: 13 executions instead of 14.
 */

atomic_int x0, x1;
int r0, r1, r2, r3;

void *t1(void *arg)
{
	r0 = atomic_fetch_add_explicit(&x0, 2, memory_order_seq_cst);
	r1 = atomic_fetch_add_explicit(&x1, 2, memory_order_relaxed);
	return NULL;
}

void *t2(void *arg)
{
	r2 = atomic_load_explicit(&x0, memory_order_acquire);
	atomic_store_explicit(&x1, 1, memory_order_relaxed);
	return NULL;
}

void *t3(void *arg)
{
	int e = 0;
	r3 = atomic_compare_exchange_weak_explicit(&x0, &e, 0, memory_order_release,
						   memory_order_relaxed);
	return NULL;
}

int main(void)
{
	pthread_t a, b, c;
	pthread_create(&a, NULL, t1, NULL);
	pthread_create(&b, NULL, t2, NULL);
	pthread_create(&c, NULL, t3, NULL);
	pthread_join(a, NULL);
	pthread_join(b, NULL);
	pthread_join(c, NULL);
	return 0;
}
