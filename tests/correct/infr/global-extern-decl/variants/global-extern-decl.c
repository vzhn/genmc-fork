#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <unistd.h>

/*
 * optind is only declared here; the interpreter binds it to the libc
 * symbol, whose value (1 before any getopt() call) is its initial value.
 */

atomic_int x;

void *thread_1(void *unused)
{
	atomic_store(&x, optind);
	return NULL;
}

int main()
{
	pthread_t t;

	pthread_create(&t, NULL, thread_1, NULL);

	int r = atomic_load(&x);
	assert(r == 0 || r == 1);

	pthread_join(t, NULL);
	return 0;
}
