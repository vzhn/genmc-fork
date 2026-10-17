#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>

/*
 * SQLite's WAL-index header (src/wal.c): readers read it without a lock.
 * The header is kept twice. walIndexWriteHdr() writes copy 1, issues a
 * memory barrier (walShmBarrier()) and writes copy 0; walIndexTryHdr()
 * reads copy 0, issues a barrier and reads copy 1, and treats the header as
 * dirty (retry later) unless the two copies are equal. Writing the copies
 * in the opposite order to the reads is what makes a torn copy show up as
 * a mismatch. The header's fields are relaxed atomics (it lives in shared
 * memory, written with memcpy) and the barriers are seq_cst fences; the
 * header's checksum is left out.
 *
 * A header here is two fields that are equal in every version. The writer
 * updates it from version 0 to 1; the reader reads it once. Whenever the
 * reader accepts the header, its two fields are equal.
 *
 * One switch per bug:
 * - BUG_SAME_ORDER: the writer writes copy 0 first, in the order the reader
 *   reads. The reader can read copy 0 half-written and then copy 1
 *   half-written the same way: the copies match and the torn header is
 *   accepted. Fails under every model;
 * - BUG_NO_READ_BARRIER / BUG_NO_WRITE_BARRIER: one of the two barriers is
 *   left out; a torn header is accepted under RC11 and IMM (passes under
 *   SC).
 */

struct header {
	atomic_int a, b;
};

struct header copy[2];

static void write_copy(struct header *h, int v)
{
	atomic_store_explicit(&h->a, v, memory_order_relaxed);
	atomic_store_explicit(&h->b, v, memory_order_relaxed);
}

static void write_header(int v)
{
#ifdef BUG_SAME_ORDER
	write_copy(&copy[0], v);
#else
	write_copy(&copy[1], v);
#endif
#ifndef BUG_NO_WRITE_BARRIER
	atomic_thread_fence(memory_order_seq_cst);
#endif
#ifdef BUG_SAME_ORDER
	write_copy(&copy[1], v);
#else
	write_copy(&copy[0], v);
#endif
}

/* Reads the header into *a, *b; returns false if it is dirty */
static bool try_header(int *a, int *b)
{
	int a0 = atomic_load_explicit(&copy[0].a, memory_order_relaxed);
	int b0 = atomic_load_explicit(&copy[0].b, memory_order_relaxed);
#ifndef BUG_NO_READ_BARRIER
	atomic_thread_fence(memory_order_seq_cst);
#endif
	int a1 = atomic_load_explicit(&copy[1].a, memory_order_relaxed);
	int b1 = atomic_load_explicit(&copy[1].b, memory_order_relaxed);

	if (a0 != a1 || b0 != b1)
		return false;
	*a = a0;
	*b = b0;
	return true;
}

static void *writer(void *unused)
{
	write_header(1);
	return NULL;
}

static void *reader(void *unused)
{
	int a, b;

	if (try_header(&a, &b))
		assert(a == b);
	return NULL;
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
