/* Issues #145 and #137: IMM orders an exclusive read, a failing CAS's
 * included, before all later events of its thread ([Rex] ; po in its
 * deps), so the load-buffering outcome below is forbidden: t0's CAS would
 * read t1's x = 1 and fail, while t1 read t0's later y = 1.
 *
 * Since #133 the dead store of the CAS's expected value no longer gave a
 * control dependency, and nothing else ordered t0's y = 1 after the CAS.
 * The backward revisit of the CAS by t1's x = 1 kept y = 1 (in the
 * prefix of x = 1), the CAS failed and lost its write part, y = 1 was no
 * longer where t0 executes it, and GenMC crashed (segmentation fault in
 * Interpreter::run()). */
#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>

atomic_int x;
atomic_int y;
int r, s;

void *t0(void *arg)
{
	int e = 0;
	atomic_compare_exchange_strong_explicit(&x, &e, 2, memory_order_relaxed, memory_order_relaxed);
	r = e;
	atomic_store_explicit(&y, 1, memory_order_relaxed);
	return NULL;
}

void *t1(void *arg)
{
	s = atomic_load_explicit(&y, memory_order_relaxed);
	atomic_store_explicit(&x, s, memory_order_relaxed);
	return NULL;
}

int main(void)
{
	pthread_t h0, h1;
	pthread_create(&h0, NULL, t0, NULL);
	pthread_create(&h1, NULL, t1, NULL);
	pthread_join(h0, NULL);
	pthread_join(h1, NULL);
	assert(!(r == 1 && s == 1));
	return 0;
}
