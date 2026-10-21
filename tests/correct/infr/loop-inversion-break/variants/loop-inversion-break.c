#include <pthread.h>
#include <stdatomic.h>

/* The header leaves the loop on the true side of its branch (the break),
 * so the condition to enter the body is the branch's negation. Loop jump
 * threading evaluated every negation to 0 (#64) and never inverted such a
 * loop: the header kept its PHI, the spin-assume pass did not recognize the
 * loop as spinning, and GenMC never terminated */

atomic_int b;

void *waiter(void *unused)
{
	for (int r = 0;;) {
		if (r)
			break;
		r = atomic_load_explicit(&b, memory_order_acquire) > 0;
	}
	return NULL;
}

void *setter(void *unused)
{
	atomic_store_explicit(&b, 1, memory_order_release);
	return NULL;
}

int main()
{
	pthread_t w, s;

	pthread_create(&w, NULL, waiter, NULL);
	pthread_create(&s, NULL, setter, NULL);
	pthread_join(w, NULL);
	pthread_join(s, NULL);
	return 0;
}
