#include <assert.h>
#include <pthread.h>

/*
 * Thread-locals initialized with the address of a global (issue #90),
 * also of one laid out after the thread-local, as the string literal is.
 * (GenMC evaluated the initializer of a thread-local before it allocated
 * the globals after it: a pointer to one of them was a host address)
 */

int g = 5;
static __thread int *tp = &g;
static __thread const char *msg = "hi";

void *thread(void *unused)
{
	assert(tp == &g && *tp == 5);
	assert(msg[0] == 'h' && msg[1] == 'i');
	tp = NULL;
	return NULL;
}

int main()
{
	pthread_t t;

	pthread_create(&t, NULL, thread, NULL);
	pthread_join(t, NULL);
	assert(tp == &g && msg[0] == 'h');
	return 0;
}
