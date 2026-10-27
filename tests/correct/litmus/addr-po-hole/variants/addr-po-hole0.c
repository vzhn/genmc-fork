#include <assert.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>

#include "../addr-po-hole.c"

int main()
{
	pthread_t t0, t1, t2;

	if (pthread_create(&t0, NULL, thread_0, NULL))
		abort();
	if (pthread_create(&t1, NULL, thread_1, NULL))
		abort();
	if (pthread_create(&t2, NULL, thread_2, NULL))
		abort();
	pthread_join(t0, NULL);
	pthread_join(t1, NULL);
	pthread_join(t2, NULL);
	assert(!(a == 1 && r == 1 && s == 0));
	return 0;
}
