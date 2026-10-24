#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>
#include <stdatomic.h>
#include <genmc.h>

/*
 * Issue #221: under IPR, a write-write race on the location of an await
 * read is reported as an error when the read is added, before it reads.
 * The invariant check of the error's graph (I6) took the read as reading
 * the initializer: under SC that is inconsistent (the read follows a
 * seq_cst write to the location in its thread), and -sc -check-invariants
 * failed. The error is on the prefix before the read.
 *
 * The Peterson lock (seq_cst): both threads write victim.
 */

atomic_bool flag[2];
atomic_int victim;

static void *thread(void *arg)
{
	int me = (intptr_t) arg;
	int other = 1 - me;

	atomic_store_explicit(&flag[me], true, memory_order_seq_cst);
	atomic_store_explicit(&victim, me, memory_order_seq_cst);
	__VERIFIER_await(!atomic_load_explicit(&flag[other], memory_order_seq_cst) ||
			 atomic_load_explicit(&victim, memory_order_seq_cst) != me);
	atomic_store_explicit(&flag[me], false, memory_order_seq_cst);
	return NULL;
}

int main()
{
	pthread_t t[2];

	for (intptr_t i = 0; i < 2; i++)
		pthread_create(&t[i], NULL, thread, (void *) i);
	for (int i = 0; i < 2; i++)
		pthread_join(t[i], NULL);
	return 0;
}
