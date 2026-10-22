#include <atomic>
#include <cassert>
#include <cstdlib>
#include <pthread.h>

/*
 * The events of the runtime headers' code are reported at the line that
 * calls it (marked @...): std::atomic's members (calls into <atomic> under
 * -disable-function-inliner), pthread_cond_wait() (which calls the mutex
 * wrappers of <pthread.h> in turn), pthread_join(), malloc() and free().
 * The assertion fails if main() waited. See scripts/check-call-site-locs.sh.
 */

std::atomic<int> ready;
int waited;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t c = PTHREAD_COND_INITIALIZER;

void *signaller(void *)
{
	ready.store(1);			/* @store */
	pthread_mutex_lock(&m);
	pthread_cond_signal(&c);	/* @signal */
	pthread_mutex_unlock(&m);
	return nullptr;
}

int main()
{
	pthread_t t;

	int *q = static_cast<int *>(std::malloc(sizeof(int)));	/* @malloc */
	pthread_create(&t, nullptr, signaller, nullptr);	/* @create */
	pthread_mutex_lock(&m);					/* @lock */
	while (ready.load() == 0) {				/* @load */
		pthread_cond_wait(&c, &m);			/* @wait */
		waited = 1;
	}
	pthread_mutex_unlock(&m);				/* @unlock */
	pthread_join(t, nullptr);				/* @join */
	std::free(q);						/* @free */
	assert(!waited);
	return 0;
}
