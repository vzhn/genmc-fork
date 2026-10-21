#include <pthread.h>
#include <assert.h>

/*
 * pthread_rwlock_trywrlock(): a writer that gets the lock writes x while
 * two readers read it under the read lock. The writer only writes if it
 * got the lock, so there is no race. After the threads are joined,
 * nothing holds the lock, and the try functions see each other's lock.
 */

pthread_rwlock_t l;
int x;

void *reader(void *unused)
{
	pthread_rwlock_rdlock(&l);
	int r = x;
	assert(r == 0 || r == 1);
	pthread_rwlock_unlock(&l);
	return NULL;
}

void *writer(void *unused)
{
	int busy = pthread_rwlock_trywrlock(&l);

#ifdef BUG_WRITE_IF_BUSY
	x = 1;
#endif
	if (busy == 0) {
#ifndef BUG_WRITE_IF_BUSY
		x = 1;
#endif
		pthread_rwlock_unlock(&l);
	}
	return NULL;
}

int main()
{
	pthread_t r1, r2, w;

	pthread_rwlock_init(&l, NULL);
	pthread_create(&r1, NULL, reader, NULL);
	pthread_create(&r2, NULL, reader, NULL);
	pthread_create(&w, NULL, writer, NULL);
	pthread_join(r1, NULL);
	pthread_join(r2, NULL);
	pthread_join(w, NULL);

	assert(pthread_rwlock_trywrlock(&l) == 0);
	assert(pthread_rwlock_tryrdlock(&l) != 0);
	pthread_rwlock_unlock(&l);
	assert(pthread_rwlock_tryrdlock(&l) == 0);
	assert(pthread_rwlock_trywrlock(&l) != 0);
	pthread_rwlock_unlock(&l);
	return 0;
}
