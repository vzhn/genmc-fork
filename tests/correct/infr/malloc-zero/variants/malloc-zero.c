#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <genmc.h>

/*
 * malloc(0) and calloc(0, n) are legal: each returns a unique pointer (or a
 * null one) that may be freed but not accessed. GenMC's own allocator
 * accepts size 0 as well.
 */

void *thread_1(void *unused)
{
	char *p = malloc(0);
	char *q = calloc(0, sizeof(int));

	assert(p != q);
	free(p);
	free(q);
	return NULL;
}

int main()
{
	pthread_t t;

	pthread_create(&t, NULL, thread_1, NULL);

	void *r = __VERIFIER_malloc(0);
	char *s = malloc(1);

	assert(r != s);
	*s = 1;
	free(r);
	free(s);
	pthread_join(t, NULL);
	return 0;
}
