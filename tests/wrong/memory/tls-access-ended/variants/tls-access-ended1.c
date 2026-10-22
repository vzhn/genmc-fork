#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>

#include "../tls-access-ended.c"

int main()
{
	pthread_t t1;

	if (pthread_create(&t1, NULL, thread1, NULL))
		abort();
	if (pthread_join(t1, NULL))
		abort();

	thread2(NULL);
	return 0;
}
