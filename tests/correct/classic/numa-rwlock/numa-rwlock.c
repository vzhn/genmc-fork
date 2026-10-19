#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>

/*
 * The NUMA-aware readers-writer lock from the kernel benchmarks of the
 * ShflLock work (Kashyap et al., SOSP 2019; cmcsmcsrw.c in the
 * sslab-gatech/shfllock repository), reduced to its reader/writer
 * handshake. Writers queue on a cohort MCS lock whose global tail tells
 * readers that a writer is in; a writer then waits for the per-node
 * reader counters to drain. A reader waits until there is no writer
 * tail, increments its node's counter, and checks the tail again; if a
 * writer arrived in between, it backs off and retries. Read unlock
 * decrements the counter.
 *
 * The writer queue is reduced to a single writer swapping itself into the
 * tail; the kernel's xchg() is fully ordered, so it is an SC exchange
 * followed by an SC fence. One NUMA node, so one counter.
 *
 * The handshake is store buffering: the reader stores to the counter and
 * loads the tail, the writer stores to the tail and loads the counter.
 * Both sides need a full barrier between the two. The reader also needs
 * acquire order on its final tail check, to see the last writer's
 * critical section, and release order on its decrement, so the next
 * writer sees its critical section.
 *
 * One writer updates a plain variable and NREADERS readers read it; with
 * the lock, the accesses never race.
 *
 * One switch per missing ordering in the original, which uses only
 * relaxed operations on the reader side:
 * - BUG_RELAXED_READ_LOCK: read lock as in the original: a relaxed
 *   increment and relaxed tail reads, no barrier. Reader and writer can
 *   both miss each other's store and enter together. VSync found this
 *   (Oberhauser et al., ASPLOS 2021, Sec. 6.5).
 * - BUG_RELAXED_READ_UNLOCK: read unlock as in the original, a relaxed
 *   decrement. The writer can see the counter drop to zero before the
 *   reader's critical-section load is done, and its update races with it.
 */

#ifndef NREADERS
# define NREADERS 1
#endif

struct writer_node {
	int unused;
};

_Atomic(struct writer_node *) gtail;
atomic_int active_readers;
struct writer_node writer_node;
int data;

static void write_lock(void)
{
	atomic_exchange_explicit(&gtail, &writer_node, memory_order_seq_cst);
	atomic_thread_fence(memory_order_seq_cst);
	/* wait_for_active_readers() */
	__VERIFIER_await(atomic_load_explicit(&active_readers, memory_order_acquire) == 0);
}

static void write_unlock(void)
{
	atomic_store_explicit(&gtail, NULL, memory_order_release);
}

static void read_lock(void)
{
	for (;;) {
		__VERIFIER_await(atomic_load_explicit(&gtail, memory_order_relaxed) == NULL);
		atomic_fetch_add_explicit(&active_readers, 1, memory_order_relaxed);
#ifdef BUG_RELAXED_READ_LOCK
		if (atomic_load_explicit(&gtail, memory_order_relaxed) == NULL)
			break;
#else
		atomic_thread_fence(memory_order_seq_cst);
		if (atomic_load_explicit(&gtail, memory_order_acquire) == NULL)
			break;
#endif
		atomic_fetch_sub_explicit(&active_readers, 1, memory_order_relaxed);
	}
}

static void read_unlock(void)
{
#ifdef BUG_RELAXED_READ_UNLOCK
	atomic_fetch_sub_explicit(&active_readers, 1, memory_order_relaxed);
#else
	atomic_fetch_sub_explicit(&active_readers, 1, memory_order_release);
#endif
}

static void *writer(void *arg)
{
	write_lock();
	data++;
	write_unlock();
	return NULL;
}

static void *reader(void *arg)
{
	read_lock();
	int r = data;
	(void) r;
	read_unlock();
	return NULL;
}

int main()
{
	pthread_t w, r[NREADERS];

	if (pthread_create(&w, NULL, writer, NULL))
		abort();
	for (int i = 0; i < NREADERS; i++)
		if (pthread_create(&r[i], NULL, reader, NULL))
			abort();
	if (pthread_join(w, NULL))
		abort();
	for (int i = 0; i < NREADERS; i++)
		if (pthread_join(r[i], NULL))
			abort();
	return 0;
}
