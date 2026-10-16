#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <genmc.h>

/*
 * A wait inside a retry loop that can never finish: the producer adds to
 * another counter, so the consumer waits for a token forever. A wait nested
 * in a loop must still be reported under -check-liveness.
 */

atomic_int tokens;
atomic_int other;

static bool take(void)
{
	int t = atomic_load(&tokens);

	return t > 0 && atomic_compare_exchange_strong(&tokens, &t, t - 1);
}

void *consumer(void *unused)
{
	for (;;) {
		if (take())
			break;
		__VERIFIER_await(atomic_load(&tokens) > 0);
	}
	return NULL;
}

void *producer(void *unused)
{
	atomic_fetch_add(&other, 1);
	return NULL;
}

int main()
{
	pthread_t c, p;

	if (pthread_create(&c, NULL, consumer, NULL))
		abort();
	if (pthread_create(&p, NULL, producer, NULL))
		abort();
	return 0;
}
