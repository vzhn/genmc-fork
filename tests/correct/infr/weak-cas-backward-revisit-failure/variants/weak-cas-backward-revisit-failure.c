#include <pthread.h>
#include <stdatomic.h>

/*
 * t2's CAS revisits t1's weak CAS, which then succeeds reading from a write
 * added after it. The spurious failure of that instance is queued like any
 * other, as a revisit of the read; its restriction cut the graph to the
 * read's stamp, which removed the write the read reads from: INTERNAL
 * FAILURE under rc11 and imm (a read without a write outside an error
 * replay), and 4 executions instead of 5 under sc.
 */

atomic_int x;
int r1, r2;

void *t1(void *arg)
{
	atomic_store_explicit(&x, 2, memory_order_seq_cst);
	int e = 1;
	r1 = atomic_compare_exchange_weak_explicit(&x, &e, 1, memory_order_acquire,
						   memory_order_relaxed);
	return NULL;
}

void *t2(void *arg)
{
	int e = 2;
	r2 = atomic_compare_exchange_weak_explicit(&x, &e, 1, memory_order_seq_cst,
						   memory_order_seq_cst);
	return NULL;
}

int main(void)
{
	pthread_t a, b;
	pthread_create(&a, NULL, t1, NULL);
	pthread_create(&b, NULL, t2, NULL);
	pthread_join(a, NULL);
	pthread_join(b, NULL);
	return 0;
}
