/* A seqlock reader: it retries when the sequence number is odd (continue) or
 * has changed (reaching the end of the body), and both paths reach the latch
 * through blocks that only jump to it. The latch's failing assumption has to
 * get past them to the branches, so that the reads of the sequence number,
 * where a failed iteration blocks, are revisited in place (#92: each write a
 * blocked iteration could read explored a copy of the blocked execution) */

#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>

atomic_int seq;
atomic_int data;

void *reader(void *arg)
{
	int s, d;

	while (1) {
		s = atomic_load_explicit(&seq, memory_order_acquire);
		if (s & 1)
			continue;
		d = atomic_load_explicit(&data, memory_order_acquire);
		if (atomic_load_explicit(&seq, memory_order_relaxed) == s)
			break;
	}
	assert(d == s >> 1);
	return NULL;
}

void *writer(void *arg)
{
	for (int i = 1; i <= 2; i++) {
		int s = atomic_load_explicit(&seq, memory_order_relaxed);
		atomic_store_explicit(&seq, s + 1, memory_order_relaxed);
		atomic_store_explicit(&data, i, memory_order_release);
		atomic_store_explicit(&seq, s + 2, memory_order_release);
	}
	return NULL;
}

int main()
{
	pthread_t t1, t2;

	pthread_create(&t1, NULL, reader, NULL);
	pthread_create(&t2, NULL, writer, NULL);
	return 0;
}
