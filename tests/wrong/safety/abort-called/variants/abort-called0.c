#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>

/*
 * abort() is an error of the program, reported in the execution that calls
 * it: here, the one where the main thread sees the flag set.
 */

atomic_int flag;

void *thread_1(void *unused)
{
	atomic_store(&flag, 1);
	return NULL;
}

int main()
{
	pthread_t t;

	pthread_create(&t, NULL, thread_1, NULL);
	if (atomic_load(&flag))
		abort();
	pthread_join(t, NULL);
	return 0;
}
