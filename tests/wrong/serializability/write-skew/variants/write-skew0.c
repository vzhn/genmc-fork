#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <genmc.h>

/*
 * Write skew: two on-call doctors, at least one of whom must stay on call.
 * Each region checks that both are on call before it takes its own doctor
 * off. Each region reads the variable the other one writes and no variable
 * is written twice, so there is no write-write conflict; yet when both read
 * the initial values, each read is fr-before the other region's write: the
 * cycle -check-serializability must report. (No assertion fails here: the
 * check reports the anomaly before the invariant is broken in a test.)
 */

atomic_int alice = 1;
atomic_int bob = 1;

void *alice_leaves(void *unused)
{
	__VERIFIER_serial_begin();
	if (atomic_load_explicit(&alice, memory_order_relaxed) +
		    atomic_load_explicit(&bob, memory_order_relaxed) ==
	    2)
		atomic_store_explicit(&alice, 0, memory_order_relaxed);
	__VERIFIER_serial_end();
	return NULL;
}

void *bob_leaves(void *unused)
{
	__VERIFIER_serial_begin();
	if (atomic_load_explicit(&alice, memory_order_relaxed) +
		    atomic_load_explicit(&bob, memory_order_relaxed) ==
	    2)
		atomic_store_explicit(&bob, 0, memory_order_relaxed);
	__VERIFIER_serial_end();
	return NULL;
}

int main()
{
	pthread_t t1, t2;

	if (pthread_create(&t1, NULL, alice_leaves, NULL))
		abort();
	if (pthread_create(&t2, NULL, bob_leaves, NULL))
		abort();
	if (pthread_join(t1, NULL))
		abort();
	if (pthread_join(t2, NULL))
		abort();
	return 0;
}
