#include <stdlib.h>
#include <pthread.h>

/* free() of a global (from another thread) is an invalid free, not an internal failure */

struct s {
	int x;
} g;

void *thread_1(void *unused)
{
	free(&g);
	return NULL;
}

int main()
{
	pthread_t t1;

	pthread_create(&t1, NULL, thread_1, NULL);
	return 0;
}
