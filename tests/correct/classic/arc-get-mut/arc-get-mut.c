#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>

/*
 * Rust's Arc::get_mut (library/alloc/src/sync.rs): it returns a mutable
 * reference to the shared data only if this Arc is the only reference.
 * is_unique() locks the weak count (a CAS from 1 to usize::MAX, acquire),
 * loads the strong count, and unlocks the weak count (a release store).
 * Dropping an Arc decrements the strong count with release.
 *
 * Thread B holds a clone of A's Arc: it reads the data and drops its clone.
 * Thread A calls get_mut and, if it gets the data, writes it. Without a
 * switch the strong count is loaded with acquire, so a strong count of 1
 * means A synchronizes with B's drop and B's read comes before A's write.
 *
 * BUG_RELAXED_STRONG: Rust before the fix of issue #51780 (found by
 * RustBelt, 2018; Ralf Jung, The Tale of a Bug in Arc) loaded the strong
 * count relaxed. A can see the count 1 without synchronizing with B's
 * release decrement, and its write races with B's read. Passes under SC.
 */

#define LOCKED UINT_MAX

struct arc_inner {
	atomic_uint strong;
	atomic_uint weak;
	int data;
};

struct arc_inner inner = { 2, 1, 0 };	/* two strong references */
int b_read;

static bool is_unique(struct arc_inner *in)
{
	unsigned int one = 1;

	if (!atomic_compare_exchange_strong_explicit(&in->weak, &one, LOCKED,
						     memory_order_acquire,
						     memory_order_relaxed))
		return false;
#ifdef BUG_RELAXED_STRONG
	bool unique = atomic_load_explicit(&in->strong, memory_order_relaxed) == 1;
#else
	bool unique = atomic_load_explicit(&in->strong, memory_order_acquire) == 1;
#endif
	atomic_store_explicit(&in->weak, 1, memory_order_release);
	return unique;
}

static int *get_mut(struct arc_inner *in)
{
	return is_unique(in) ? &in->data : NULL;
}

static void drop(struct arc_inner *in)
{
	if (atomic_fetch_sub_explicit(&in->strong, 1, memory_order_release) == 1) {
		atomic_thread_fence(memory_order_acquire);
		/* the last reference: the data would be dropped here */
	}
}

static void *thread_a(void *unused)
{
	int *p = get_mut(&inner);

	if (p)
		*p = 42;
	return NULL;
}

static void *thread_b(void *unused)
{
	b_read = inner.data;
	drop(&inner);
	return NULL;
}

int main()
{
	pthread_t a, b;

	if (pthread_create(&a, NULL, thread_a, NULL))
		abort();
	if (pthread_create(&b, NULL, thread_b, NULL))
		abort();
	if (pthread_join(a, NULL))
		abort();
	if (pthread_join(b, NULL))
		abort();
	assert(b_read == 0);	/* B read before A could write */
	return 0;
}
