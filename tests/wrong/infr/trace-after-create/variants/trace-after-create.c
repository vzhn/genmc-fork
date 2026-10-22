#include <assert.h>
#include <pthread.h>

/*
 * -print-error-trace must list the second pthread_create() and the
 * pthread_join(), although each comes right after a thread's creation:
 * neither is the store of a thread's ID, which a pthread_t kept in a
 * register does not have. See expected-output.in.
 */

int x;

void *writer(void *unused)
{
	x = 1;
	return NULL;
}

void *idle(void *unused) { return NULL; }

int main()
{
	pthread_t t1, t2;

	pthread_create(&t1, NULL, writer, NULL);
	pthread_create(&t2, NULL, idle, NULL);
	pthread_join(t1, NULL);
	assert(x == 0);
	return 0;
}
