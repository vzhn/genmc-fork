#include <stdlib.h>
#include <pthread.h>
#include <assert.h>

#include "../names-init-padding.c"

int main()
{
	pthread_t t1;

	if (pthread_create(&t1, NULL, thread_1, NULL))
		abort();

	return 0;
}
