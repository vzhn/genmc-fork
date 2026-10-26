#include <stdlib.h>
#include <pthread.h>
#include "lib/store.h"

/*
 * The two stores of x race; both are made by store_x(), defined in
 * lib/store.h. scripts/check-library-dir.sh checks that the race is
 * reported at store.h's line without -library-dir, and at the lines of the
 * calls below with it.
 */

int x;

void *worker(void *arg)
{
	store_x(1);
	return NULL;
}

int main()
{
	pthread_t t;

	pthread_create(&t, NULL, worker, NULL);
	store_x(2);
	pthread_join(t, NULL);
	return 0;
}
