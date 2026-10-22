#include <pthread.h>
#include <assert.h>

/*
 * pthread_atfork(): two libraries register handlers that take their lock
 * before a fork and release it after, as pthreadpool and tevent do. The
 * forking thread runs the prepare handlers last registered first, and the
 * parent handlers first registered first. A worker takes the same locks,
 * in the order that agrees with the prepare handlers, so there is no
 * deadlock. A late registration, concurrent with the fork, has either
 * both its handlers run or neither.
 */

pthread_mutex_t a = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t b = PTHREAD_MUTEX_INITIALIZER;
int log_[8];
int nlog;
int late_prepared;
int late_parented;

void prepare_a(void) { pthread_mutex_lock(&a); log_[nlog++] = 1; }
void parent_a(void) { log_[nlog++] = 2; pthread_mutex_unlock(&a); }
void prepare_b(void) { pthread_mutex_lock(&b); log_[nlog++] = 3; }
void parent_b(void) { log_[nlog++] = 4; pthread_mutex_unlock(&b); }
void prepare_late(void) { late_prepared = 1; }
void parent_late(void) { late_parented = 1; }

void *worker(void *unused)
{
#ifdef BUG_LOCK_ORDER
	pthread_mutex_lock(&a);
	pthread_mutex_lock(&b);
#else
	pthread_mutex_lock(&b);
	pthread_mutex_lock(&a);
#endif
	pthread_mutex_unlock(&a);
	pthread_mutex_unlock(&b);
	return NULL;
}

void *late(void *unused)
{
	pthread_atfork(prepare_late, parent_late, NULL);
	return NULL;
}

int main()
{
	pthread_t w, l;

	/* "a" is registered first, so its prepare handler runs last */
	pthread_atfork(prepare_a, parent_a, NULL);
	pthread_atfork(prepare_b, parent_b, NULL);

	pthread_create(&w, NULL, worker, NULL);
	pthread_create(&l, NULL, late, NULL);

	__VERIFIER_atfork_prepare();
	/* fork() */
	__VERIFIER_atfork_parent();

	pthread_join(w, NULL);
	pthread_join(l, NULL);

	assert(late_prepared == late_parented);
	assert(nlog == 4);
	assert(log_[0] == 3 && log_[1] == 1 && log_[2] == 2 && log_[3] == 4);
	return 0;
}
