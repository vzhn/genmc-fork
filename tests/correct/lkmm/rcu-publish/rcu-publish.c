#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <lkmm.h>

/*
 * Publishing an initialised object to RCU readers: the writer fills it in
 * and publishes it with rcu_assign_pointer(), a release store; the reader
 * finds it with rcu_dereference() and sees the initialised fields, as the
 * address dependency orders its read of the field after the pointer.
 *
 * BUG_WRITE_ONCE: the writer publishes with WRITE_ONCE(), which orders
 * nothing: the reader can find the object and read the field it had before
 * the initialisation.
 */

struct obj {
	int a;
	int b;
};

struct obj o;
struct obj *gp;

void *writer(void *unused)
{
	WRITE_ONCE(o.a, 1);
	WRITE_ONCE(o.b, 2);
#ifdef BUG_WRITE_ONCE
	WRITE_ONCE(gp, &o);
#else
	rcu_assign_pointer(gp, &o);
#endif
	return NULL;
}

void *reader(void *unused)
{
	rcu_read_lock();
	struct obj *p = rcu_dereference(gp);
	if (p)
		assert(READ_ONCE(p->a) == 1 && READ_ONCE(p->b) == 2);
	rcu_read_unlock();
	return NULL;
}

int main()
{
	pthread_t w, r;

	if (pthread_create(&w, NULL, writer, NULL))
		abort();
	if (pthread_create(&r, NULL, reader, NULL))
		abort();
	if (pthread_join(w, NULL))
		abort();
	if (pthread_join(r, NULL))
		abort();
	return 0;
}
