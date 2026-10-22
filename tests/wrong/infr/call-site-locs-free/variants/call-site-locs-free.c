#include <pthread.h>
#include <stdlib.h>

/*
 * The events of the runtime headers' wrappers (mutex calls, calloc(),
 * free(), pthread_create()) are reported at the line that calls them (each
 * marked @...), not at the header's: thread 2 locks a mutex thread 1 freed.
 * See scripts/check-call-site-locs.sh.
 */

struct pool {
	pthread_mutex_t mutex;
	int jobs;
};
struct pool *p;
pthread_mutex_t g = PTHREAD_MUTEX_INITIALIZER;

void *a(void *unused)
{
	pthread_mutex_lock(&g);			/* @lock-g */
	p->jobs = 1;
	pthread_mutex_unlock(&g);		/* @unlock-g */
	pthread_mutex_destroy(&p->mutex);	/* @destroy */
	free(p);				/* @free */
	return NULL;
}

void *b(void *unused)
{
	pthread_mutex_lock(&p->mutex);		/* @lock-freed */
	return NULL;
}

int main()
{
	pthread_t x, y;

	p = calloc(1, sizeof(*p));		/* @calloc */
	pthread_mutex_init(&p->mutex, NULL);	/* @init */
	pthread_create(&x, NULL, a, NULL);	/* @create-a */
	pthread_create(&y, NULL, b, NULL);	/* @create-b */
	return 0;
}
