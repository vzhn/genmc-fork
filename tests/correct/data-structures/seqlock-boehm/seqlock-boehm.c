#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>

/*
 * A seqlock in C11 as in Hans Boehm, Can Seqlocks Get Along With
 * Programming Language Memory Models? (MSPC 2012). The data are relaxed
 * atomics. The writer makes the sequence number odd, issues a release
 * fence, writes the data and makes the number even again with a release
 * store. The reader loads the number with acquire, reads the data, issues
 * an acquire fence and loads the number again; it keeps the data only if
 * both numbers are equal and even.
 *
 * A writer writes (1, 1) and then (2, 2) over (0, 0); a reader takes one
 * snapshot, which must be one of the three pairs, never a mix.
 *
 * One switch per bug, both incorrect versions the paper discusses:
 * - BUG_NO_READER_FENCE: no acquire fence before the second load of the
 *   number. The data reads are not ordered before it, so the reader can
 *   see new data with an old, unchanged number;
 * - BUG_NO_WRITER_FENCE: no release fence after making the number odd. The
 *   data writes are not ordered after it, so a reader can see new data and
 *   still the old even number.
 * Both pass under SC.
 */

atomic_uint seq;
atomic_int data1, data2;

static void write_pair(int v)
{
	unsigned int s = atomic_load_explicit(&seq, memory_order_relaxed);

	atomic_store_explicit(&seq, s + 1, memory_order_relaxed);
#ifndef BUG_NO_WRITER_FENCE
	atomic_thread_fence(memory_order_release);
#endif
	atomic_store_explicit(&data1, v, memory_order_relaxed);
	atomic_store_explicit(&data2, v, memory_order_relaxed);
	atomic_store_explicit(&seq, s + 2, memory_order_release);
}

static void *writer(void *unused)
{
	write_pair(1);
	write_pair(2);
	return NULL;
}

static void *reader(void *unused)
{
	for (;;) {
		unsigned int s0 = atomic_load_explicit(&seq, memory_order_acquire);
		int r1 = atomic_load_explicit(&data1, memory_order_relaxed);
		int r2 = atomic_load_explicit(&data2, memory_order_relaxed);

#ifndef BUG_NO_READER_FENCE
		atomic_thread_fence(memory_order_acquire);
#endif
		unsigned int s1 = atomic_load_explicit(&seq, memory_order_relaxed);

		if (s0 == s1 && !(s0 & 1)) {
			assert(r1 == r2);
			return NULL;
		}
	}
}

int main()
{
	pthread_t w, r;

	if (pthread_create(&w, NULL, writer, NULL))
		abort();
	if (pthread_create(&r, NULL, reader, NULL))
		abort();
	if (pthread_join(w, NULL))
		abort();
	if (pthread_join(r, NULL))
		abort();
	return 0;
}
