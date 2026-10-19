#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <lkmm.h>

/*
 * The index of srcu_down_read() handed to srcu_up_read() in another thread
 * through thread creation (JOIN undefined: the created thread's argument)
 * or join (JOIN: the joined thread's return value; with EXIT, the value it
 * passes to pthread_exit()). The unlock matches the
 * lock by the data dependency of its value, which GenMC carries through
 * creation and join as through a write and a read (herd7 has no threads).
 * A writer makes 2 executions.
 */

struct srcu_struct s;
int x;

#ifndef JOIN
void *closer(void *arg)
{
	srcu_up_read(&s, (int) (intptr_t) arg);
	return NULL;
}
#else
void *opener(void *unused)
{
	int idx = srcu_down_read(&s);
	int r = READ_ONCE(x);
	(void) r;
#ifdef EXIT
	pthread_exit((void *) (intptr_t) idx);
#endif
	return (void *) (intptr_t) idx;
}
#endif

void *writer(void *unused)
{
	WRITE_ONCE(x, 1);
	return NULL;
}

int main()
{
	pthread_t t, w;

	if (pthread_create(&w, NULL, writer, NULL))
		abort();
#ifndef JOIN
	int idx = srcu_down_read(&s);
	int r = READ_ONCE(x);
	(void) r;
	if (pthread_create(&t, NULL, closer, (void *) (intptr_t) idx))
		abort();
	if (pthread_join(t, NULL))
		abort();
#else
	void *ret;
	if (pthread_create(&t, NULL, opener, NULL))
		abort();
	if (pthread_join(t, &ret))
		abort();
	srcu_up_read(&s, (int) (intptr_t) ret);
#endif
	return 0;
}
