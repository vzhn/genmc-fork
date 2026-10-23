/* -lkmm, without race detection: a store through a NULL pointer (read
 * without the lock) is an access to non-allocated memory. The store is
 * checked before it is placed in coherence, and reporting the error
 * evaluates LKMM on the graph: its coherence relation took the coherence
 * successors of the unplaced store, and GenMC crashed (#202).
 */
#include <pthread.h>
#include <stddef.h>

struct disk { int queue; };
struct bdev { pthread_mutex_t m; int openers; struct disk *disk; };

struct disk d;
struct bdev b = { PTHREAD_MUTEX_INITIALIZER, 1, &d };

void *iterate(void *u)
{
	struct disk *p = b.disk;	/* unlocked */
	p->queue = 1;
	return NULL;
}

void *close_(void *u)
{
	pthread_mutex_lock(&b.m);
	int o = b.openers - 1;
	b.openers = o;
	if (!o)
		b.disk = NULL;
	pthread_mutex_unlock(&b.m);
	return NULL;
}

int main()
{
	pthread_t t1, t2;
	pthread_create(&t1, NULL, iterate, NULL);
	pthread_create(&t2, NULL, close_, NULL);
	return 0;
}
