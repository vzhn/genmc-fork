/* 110 executions, no error (as under rc11 and sc with -disable-sr). An
 * object has room for one holder: a new one evicts the old, which its
 * holder frees on its last put(). A revisit keeps a holder's free while
 * the thread re-runs and adds its accesses to the object before it; they
 * were taken as accesses added after the free (issue #96): "Attempt to
 * access freed memory". */
#include <pthread.h>
#include <stdlib.h>
#include <lkmm.h>

struct obj { int ref; int zombie; int *buf; pthread_rwlock_t rw; };
static struct obj *slot;	/* room for one object: a new one evicts */
static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

static void put(struct obj *e)
{
	pthread_mutex_lock(&m);
	if (--e->ref == 0 && e->zombie) {
		free(e->buf);
		free(e);
	}
	pthread_mutex_unlock(&m);
}

static void *user(void *arg)
{
	struct obj *e = calloc(1, sizeof(*e));

	pthread_rwlock_init(&e->rw, NULL);
	pthread_mutex_lock(&m);
	if (slot) {
		struct obj *old = slot;

		old->zombie = 1;
		slot = NULL;
		if (old->ref == 0) {
			free(old->buf);
			free(old);
		}
	}
	e->ref = 1;
	slot = e;
	pthread_mutex_unlock(&m);

	int *b = malloc(sizeof(int));	/* only the holder sets buf */
	pthread_rwlock_wrlock(&e->rw);
	e->buf = b;
	*b = 1;
	pthread_rwlock_unlock(&e->rw);
	put(e);
	return NULL;
}

static void *reader(void *arg)
{
	struct obj *e;

	pthread_mutex_lock(&m);
	e = slot;
	if (e)
		e->ref++;
	pthread_mutex_unlock(&m);
	if (!e)
		return NULL;
	pthread_rwlock_rdlock(&e->rw);
	if (e->buf)
		(void)*e->buf;
	pthread_rwlock_unlock(&e->rw);
	put(e);
	return NULL;
}

int main()
{
	pthread_t a, b, c;

	pthread_create(&a, NULL, user, NULL);
	pthread_create(&b, NULL, user, NULL);
	pthread_create(&c, NULL, reader, NULL);
	pthread_join(a, NULL);
	pthread_join(b, NULL);
	pthread_join(c, NULL);
	if (slot) {
		free(slot->buf);
		free(slot);
	}
	return 0;
}
