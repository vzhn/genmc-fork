#include <stdlib.h>
#include <pthread.h>

/*
 * main writes x right after creating the thread that writes it too. The race
 * is reported at the worker's store, which the replay runs up to; main's
 * store is not in that store's prefix and was not replayed, so the graph
 * printed it with neither the variable's name nor a source location. A tool
 * that reads the report (an SV-COMP witness) needs both accesses' lines.
 */

int x;

void *worker(void *arg)
{
	x = 1;
	return NULL;
}

int main()
{
	pthread_t t;

	pthread_create(&t, NULL, worker, NULL);
	x = 5;
	pthread_join(t, NULL);
	return 0;
}
