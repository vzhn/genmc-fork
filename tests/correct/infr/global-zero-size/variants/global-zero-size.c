#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>

/*
 * Globals of zero size (an empty struct, a zero-length array) still get
 * an address of their own, distinct from that of any other global.
 */

struct empty {} e;
int z[0];
atomic_int x;

void *thread_1(void *unused)
{
	atomic_store(&x, 1);
	return NULL;
}

int main()
{
	pthread_t t;

	pthread_create(&t, NULL, thread_1, NULL);

	assert((void *)&e != (void *)&x);
	assert((void *)z != (void *)&x);
	int r = atomic_load(&x);
	assert(r == 0 || r == 1);

	pthread_join(t, NULL);
	return 0;
}
