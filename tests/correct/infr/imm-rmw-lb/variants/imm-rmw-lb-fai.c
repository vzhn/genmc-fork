/* IMM orders the read of an RMW (an exclusive read) before all later
 * events of its thread ([Rex] ; po in its deps), so the load-buffering
 * outcome below is forbidden: t0's fetch_add would read t1's x = 1, while
 * t1 read t0's later y = 1 (the ar cycle Rex(x) ->ppo W(y) ->rfe R(y)
 * ->data W(x) ->rfe Rex(x)).
 *
 * GenMC's IMM ordered an RMW's read before its own write part only (rmw in
 * its deps), and reported the assertion as violated; a CAS has the
 * ordering since the fix of #137 and #145. */
#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>

atomic_int x;
atomic_int y;
int r, s;

void *t0(void *arg)
{
	r = atomic_fetch_add_explicit(&x, 1, memory_order_relaxed);
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
