/* Issue #121: under IMM, one execution was lost (23 instead of 24, as under
 * rc11 and sc): t2's fetch_add reads the initial value, t2's x0 = 3 and then
 * t1's x0 = 2 follow its write in coherence, t3 reads 3 and writes x1 = 1,
 * and t1 reads that 1. It comes from the backward revisit of t1's load by
 * t3's x1 = 1, from the graph where t2's x0 = 3 (read by t3, so kept) is
 * placed before t1's x0 = 2 and t2's fetch_add (not in the prefix) is
 * deleted. The fetch_add had read the initial value while x0 = 2 was the
 * co-latest write, so its added-maximally flag was off and the revisit was
 * rejected. But the kept x0 = 3, added after it, bounds it: the fetch_add
 * precedes it in coherence, so the initial value is the latest write it can
 * read, and no other graph leads to the same revisit. */
#include <pthread.h>
#include <stdatomic.h>

atomic_int x0, x1;
int r1, r2, r3;

void *t1(void *arg)
{
	atomic_store_explicit(&x0, 2, memory_order_relaxed);
	r1 = atomic_load_explicit(&x1, memory_order_relaxed);
	return NULL;
}

void *t2(void *arg)
{
	r2 = atomic_fetch_add_explicit(&x0, 1, memory_order_relaxed);
	atomic_store_explicit(&x0, 3, memory_order_relaxed);
	return NULL;
}

void *t3(void *arg)
{
	r3 = atomic_load_explicit(&x0, memory_order_relaxed);
	atomic_store_explicit(&x1, 1, memory_order_seq_cst);
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
