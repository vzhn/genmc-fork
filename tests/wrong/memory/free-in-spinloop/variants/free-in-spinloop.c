#include <pthread.h>
#include <stdlib.h>
#include <stdatomic.h>

/* main frees the block in a loop until the other thread has run: the
 * second iteration frees it again. Freeing is a side effect of an
 * iteration, so the loop is no spinloop */

atomic_int done;

void *thread(void *unused)
{
	atomic_store(&done, 1);
	return NULL;
}

int main()
{
	pthread_t t;
	int *p = malloc(sizeof(int));

	pthread_create(&t, NULL, thread, NULL);
	while (atomic_load(&done) == 0)
		free(p);
	return 0;
}
