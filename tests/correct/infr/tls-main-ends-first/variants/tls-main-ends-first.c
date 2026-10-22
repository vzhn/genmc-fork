#include <pthread.h>
#include <stdatomic.h>

/*
 * The end of a thread frees its copies of the thread-locals, and an access
 * of another thread to them must happen before it (#90). The main thread's
 * copies last as long as the program, but when main ended, every access not
 * before its end was reported as one to them: here the read of g, which is
 * no thread-local, which main waits for without synchronizing with it
 * ("Attempt to access freed memory")
 */

static __thread int tl;
int g;
atomic_int flag;

void *thread(void *unused)
{
	tl = g;
	atomic_store_explicit(&flag, 1, memory_order_relaxed);
	return NULL;
}

int main()
{
	pthread_t t;

	pthread_create(&t, NULL, thread, NULL);
	while (!atomic_load_explicit(&flag, memory_order_relaxed))
		;
	return 0;
}
