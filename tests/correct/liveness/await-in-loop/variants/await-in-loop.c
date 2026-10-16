#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <genmc.h>

/*
 * A wait inside a retry loop: take a token, or wait for one and retry. The
 * producer always adds a token, so no wait is stuck; -check-liveness
 * (args.*.in) must not report one. The loop's own spin-start must not be
 * confused with the await's.
 */

atomic_int tokens;

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
	atomic_fetch_add(&tokens, 1);
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
