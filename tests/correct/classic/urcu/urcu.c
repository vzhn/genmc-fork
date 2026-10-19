#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>

/*
 * Userspace RCU, memory-barrier flavor (liburcu's urcu-mb; Desnoyers,
 * McKenney, Stern, Dagenais, Walpole, User-Level Implementations of
 * Read-Copy Update, IEEE TPDS 2012). The global counter gp_ctr holds a
 * phase bit. rcu_read_lock() copies gp_ctr (with a nesting count of 1)
 * into the reader's counter and issues smp_mb(); rcu_read_unlock() issues
 * smp_mb() and drops the count. synchronize_rcu() calls
 * update_counter_and_wait() twice: flip the phase, smp_mb(), and wait for
 * every reader that is inside a read-side section begun in the other
 * phase. Kernel-style primitives map to C11: smp_mb() is a seq_cst fence,
 * LOAD_SHARED/STORE_SHARED are relaxed, rcu_assign_pointer() is a release
 * store and rcu_dereference() an acquire load. Waits are awaits.
 *
 * The writer publishes a new object, synchronizes and frees the old one,
 * twice; the reader reads the current object inside one read-side section.
 *
 * BUG_SINGLE_FLIP: synchronize_rcu() flips the phase and waits only once.
 * The paper explains why that is not enough: the reader loads gp_ctr and
 * stalls before storing it; the first grace period sees it idle and ends;
 * the reader then stores the stale phase and reads the new object; after
 * the second flip that stale phase looks current, so the second grace
 * period does not wait for the reader and frees the object under it.
 */

#define GP_COUNT 1UL
#define GP_PHASE (1UL << 32)
#define NEST_MASK (GP_PHASE - 1)

struct object {
	int value;
};

atomic_ulong gp_ctr = GP_COUNT;
atomic_ulong reader_ctr;
_Atomic(struct object *) shared;

static void rcu_read_lock(void)
{
	unsigned long tmp = atomic_load_explicit(&reader_ctr, memory_order_relaxed);

	if (!(tmp & NEST_MASK)) {
		atomic_store_explicit(&reader_ctr,
				      atomic_load_explicit(&gp_ctr, memory_order_relaxed),
				      memory_order_relaxed);
		atomic_thread_fence(memory_order_seq_cst);
	} else {
		atomic_store_explicit(&reader_ctr, tmp + GP_COUNT, memory_order_relaxed);
	}
}

static void rcu_read_unlock(void)
{
	atomic_thread_fence(memory_order_seq_cst);
	atomic_store_explicit(&reader_ctr,
			      atomic_load_explicit(&reader_ctr, memory_order_relaxed) - GP_COUNT,
			      memory_order_relaxed);
}

/* Is the reader inside a read-side section begun in the other phase? */
static bool gp_ongoing(void)
{
	unsigned long v = atomic_load_explicit(&reader_ctr, memory_order_relaxed);

	return (v & NEST_MASK) &&
	       ((v ^ atomic_load_explicit(&gp_ctr, memory_order_relaxed)) & ~NEST_MASK);
}

static void update_counter_and_wait(void)
{
	atomic_store_explicit(&gp_ctr,
			      atomic_load_explicit(&gp_ctr, memory_order_relaxed) ^ GP_PHASE,
			      memory_order_relaxed);
	atomic_thread_fence(memory_order_seq_cst);
	__VERIFIER_await(!gp_ongoing());
}

static void synchronize_rcu(void)
{
	atomic_thread_fence(memory_order_seq_cst);
	update_counter_and_wait();
#ifndef BUG_SINGLE_FLIP
	atomic_thread_fence(memory_order_seq_cst);
	update_counter_and_wait();
#endif
	atomic_thread_fence(memory_order_seq_cst);
}

static struct object *new_object(int v)
{
	struct object *o = malloc(sizeof(*o));

	o->value = v;
	return o;
}

static void *writer(void *unused)
{
	for (int v = 1; v <= 2; v++) {
		struct object *old = atomic_load_explicit(&shared, memory_order_relaxed);

		atomic_store_explicit(&shared, new_object(v), memory_order_release);
		synchronize_rcu();
		free(old);
	}
	return NULL;
}

static void *reader(void *unused)
{
	rcu_read_lock();
	struct object *o = atomic_load_explicit(&shared, memory_order_acquire);
	int v = o->value;
	rcu_read_unlock();
	assert(v >= 0 && v <= 2);
	return NULL;
}

int main()
{
	pthread_t w, r;

	atomic_init(&shared, new_object(0));
	if (pthread_create(&w, NULL, writer, NULL))
		abort();
	if (pthread_create(&r, NULL, reader, NULL))
		abort();
	if (pthread_join(w, NULL))
		abort();
	if (pthread_join(r, NULL))
		abort();
	free(atomic_load_explicit(&shared, memory_order_relaxed));
	return 0;
}
