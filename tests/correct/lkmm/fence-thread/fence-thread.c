#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <lkmm.h>

/*
 * Message passing through a thread creation (or a thread's end and its
 * join) after a strong fence. GenMC's creation and join synchronise like a
 * release and an acquire, so this is the message passing of
 * WRITE_ONCE(x, 1); smp_mb(); smp_store_release(&c, 1) with the reader's
 * smp_rmb(), which LKMM forbids (pb). Under -lkmm, smp_mb() and
 * synchronize_rcu() ordered only accesses, so the reader could see y but not
 * x; the generated checker and the direct evaluation of the model
 * (-lkmm-reference) also disagreed on synchronize_rcu().
 *
 * By default the writer is main and the fence precedes the creation of the
 * thread that writes y; with JOIN it is a thread whose end main joins
 * before writing y. GP uses synchronize_rcu(), else smp_mb().
 */

int x, y;
int rx, ry;

static void fence(void)
{
#ifdef GP
	synchronize_rcu();
#else
	smp_mb();
#endif
}

void *writer_y(void *unused)
{
	WRITE_ONCE(y, 1);
	return NULL;
}

void *writer_x(void *unused)
{
	WRITE_ONCE(x, 1);
	fence();
	return NULL;
}

void *reader(void *unused)
{
	ry = READ_ONCE(y);
	smp_rmb();
	rx = READ_ONCE(x);
	return NULL;
}

int main()
{
	pthread_t r, w;

	if (pthread_create(&r, NULL, reader, NULL))
		abort();
#ifdef JOIN
	if (pthread_create(&w, NULL, writer_x, NULL))
		abort();
	if (pthread_join(w, NULL))
		abort();
	WRITE_ONCE(y, 1);
#else
	WRITE_ONCE(x, 1);
	fence();
	if (pthread_create(&w, NULL, writer_y, NULL))
		abort();
	if (pthread_join(w, NULL))
		abort();
#endif
	if (pthread_join(r, NULL))
		abort();
	assert(!(ry == 1 && rx == 0));
	return 0;
}
