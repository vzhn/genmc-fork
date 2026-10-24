/*
 * exit() ends the calling thread, from any depth of calls, and in main
 * runs the atexit() handlers first (it used to crash the interpreter,
 * which went on with the thread's empty stack). The code after it is
 * never reached. A thread that calls it ends; the others go on.
 */
#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>

atomic_int x;
atomic_int handled;

void handler(void)
{
	/* After main joined the thread */
	assert(atomic_load(&x) == 1);
	atomic_store(&handled, 1);
}

void leave(int code)
{
	exit(code);
	assert(0);
}

void *thread(void *unused)
{
	atomic_store(&x, 1);
	leave(3);
	assert(0);
	return NULL;
}

int main()
{
	pthread_t t;

	atexit(handler);
	if (pthread_create(&t, NULL, thread, NULL))
		abort();
	if (pthread_join(t, NULL))
		abort();
	leave(0);
	assert(0);
	return 0;
}
