#include <pthread.h>
#include <stdatomic.h>

/*
 * MPU+rel+acq under TSO, race-free: TSO's hb (po | [REL];rf;[ACQ]) has no
 * release sequences, so the FAI is acq_rel to carry thread_1's release to
 * thread_3 (with a relaxed FAI, the writes to x race under -tso). Under
 * -tso -check-invariants, the TSO checker's whole-graph check (I5) walks the
 * plain writes to x; wrong/racy/MPU+rel+rlx covers the racing plain write,
 * which is not in coherence.
 */

int x;
atomic_int y;

void *thread_1(void *unused)
{
	x = 1;
	atomic_store_explicit(&y, 1, memory_order_release);
	return NULL;
}

void *thread_2(void *unused)
{
	atomic_fetch_add_explicit(&y, 1, memory_order_acq_rel);
	return NULL;
}

void *thread_3(void *unused)
{
	if (atomic_load_explicit(&y, memory_order_acquire) > 1)
		x = 2;
	return NULL;
}

int main()
{
	pthread_t t1, t2, t3;

	pthread_create(&t1, NULL, thread_1, NULL);
	pthread_create(&t2, NULL, thread_2, NULL);
	pthread_create(&t3, NULL, thread_3, NULL);
	return 0;
}
