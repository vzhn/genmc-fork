#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <genmc.h>

/*
 * An assume compares a loaded pointer with a global's address. The pass that
 * annotates the load with the values the assume rejects could not model the
 * address (#267) and aborted; the load is now left without an annotation.
 * One execution: the reader sees the writer's &slot[1].
 */

int slot[2];
_Atomic(int *) p;

void *writer(void *arg)
{
	atomic_store(&p, (int *) arg);
	return NULL;
}

int main()
{
	pthread_t t;

	pthread_create(&t, NULL, writer, &slot[1]);
	if (atomic_load(&p) != &slot[1])
		__VERIFIER_assume(0);
	pthread_join(t, NULL);
	return 0;
}
