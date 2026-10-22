#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>

#include "../tls-access-ended.c"

atomic_int x;

/* Thread 1 waits for thread 2 to be done before it ends, but through a
 * relaxed flag, which does not order the access before the end. Thread 1
 * ends after the access in every execution */
void *thread1_wait(void *unused)
{
	thread1(NULL);
	while (!atomic_load_explicit(&x, memory_order_relaxed))
		;
	return NULL;
}

void *thread2_signal(void *unused)
{
	thread2(NULL);
	atomic_store_explicit(&x, 1, memory_order_relaxed);
	return NULL;
}

int main()
{
	pthread_t t1, t2;

	if (pthread_create(&t1, NULL, thread1_wait, NULL))
		abort();
	if (pthread_create(&t2, NULL, thread2_signal, NULL))
		abort();

	return 0;
}
