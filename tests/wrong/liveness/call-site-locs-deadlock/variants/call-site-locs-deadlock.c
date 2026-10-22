#include <pthread.h>
#include <stdlib.h>

/*
 * Two threads take two mutexes in opposite orders. The report locates each
 * lock, and the block after the lock a thread is stuck at, at the line that
 * calls pthread_mutex_lock() (marked @...), not at <pthread.h>'s.
 * See scripts/check-call-site-locs.sh.
 */

pthread_mutex_t m1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t m2 = PTHREAD_MUTEX_INITIALIZER;

void *a(void *unused)
{
	pthread_mutex_lock(&m1);	/* @a-lock-m1 */
	pthread_mutex_lock(&m2);	/* @a-lock-m2 */
	pthread_mutex_unlock(&m2);
	pthread_mutex_unlock(&m1);
	return NULL;
}

void *b(void *unused)
{
	pthread_mutex_lock(&m2);	/* @b-lock-m2 */
	pthread_mutex_lock(&m1);	/* @b-lock-m1 */
	pthread_mutex_unlock(&m1);
	pthread_mutex_unlock(&m2);
	return NULL;
}

int main()
{
	pthread_t x, y;

	pthread_create(&x, NULL, a, NULL);	/* @create-a */
	pthread_create(&y, NULL, b, NULL);	/* @create-b */
	pthread_join(x, NULL);
	pthread_join(y, NULL);
	return 0;
}
