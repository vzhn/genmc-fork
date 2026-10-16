#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>

/*
 * A spinloop on a plain variable that is never written: a liveness
 * violation whose last read before the block is non-atomic. Before printing
 * the report, GenMC replays the execution up to the error; the replay target
 * (here main's last read before its join) was erased from the replay schedule
 * with the other non-atomic accesses, so the replay never reached it and the
 * error was lost ("No errors were detected"), or the next execution aborted.
 */

int x;
atomic_int y;

void *spinner(void *unused)
{
	while (!x)
		;
	return NULL;
}

int main()
{
	pthread_t t;

	pthread_create(&t, NULL, spinner, NULL);
	atomic_store(&y, 1);
	pthread_join(t, NULL);
	return 0;
}
