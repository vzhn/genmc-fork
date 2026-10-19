#include <stdatomic.h>
#include <stdint.h>
#include <pthread.h>
#include <genmc.h>

/*
 * The spinloop ends once the other thread has added 100: c is then 200 and
 * (uint8_t)(c + 100) is 44. GenMC must wrap the addition to 8 bits when it
 * evaluates the loop's condition, or it reports a non-terminating spinloop
 * under -check-liveness (args.*.in).
 */

_Atomic(uint8_t) c = 100;

void *adder(void *unused)
{
	atomic_fetch_add(&c, 100);
	return NULL;
}

int main()
{
	pthread_t t;

	pthread_create(&t, NULL, adder, NULL);
	while ((uint8_t)(atomic_load(&c) + 100) != 44)
		;
	return 0;
}
