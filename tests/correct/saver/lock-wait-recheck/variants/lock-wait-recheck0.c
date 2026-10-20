#include <pthread.h>
#include <assert.h>

/* A waiter checks the queue twice per iteration, each time under the
 * mutex. The producer may take the mutex after the waiter's first check
 * and push before its second one: the iteration then took nothing, but the
 * next one will. With -check-liveness this is no liveness violation */

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
int qlen, taken;

static int try_take(void)
{
	int ok = 0;

	pthread_mutex_lock(&m);
	if (qlen > 0) {
		qlen--;
		ok = 1;
	}
	pthread_mutex_unlock(&m);
	return ok;
}

static int queue_empty(void)
{
	pthread_mutex_lock(&m);
	int e = qlen == 0;
	pthread_mutex_unlock(&m);
	return e;
}

void *waiter(void *unused)
{
	while (!try_take()) {
		if (!queue_empty())
			continue;
	}
	taken = 1;
	return NULL;
}

void *producer(void *unused)
{
	pthread_mutex_lock(&m);
	qlen++;
	pthread_mutex_unlock(&m);
	return NULL;
}

int main()
{
	pthread_t w, p;

	pthread_create(&w, NULL, waiter, NULL);
	pthread_create(&p, NULL, producer, NULL);
	pthread_join(w, NULL);
	pthread_join(p, NULL);
	assert(taken && qlen == 0);
	return 0;
}
