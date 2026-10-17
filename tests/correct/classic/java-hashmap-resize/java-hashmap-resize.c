#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>
#include "../../../lib/lock.h"

/*
 * Java 7's HashMap.transfer(), which resize() uses to move every entry of
 * the old table into a new one: it walks each bucket and inserts each entry
 * at the head of its new bucket, which reverses the bucket's order. Java
 * fields are relaxed atomics here (a data race on them is not undefined
 * behaviour in Java).
 *
 * One bucket holds A -> B, and both entries stay in the same bucket of the
 * new table. Two threads resize the map, each into its own new table, as
 * two threads that both see the table full do. Afterwards no bucket of
 * either new table may contain a cycle (a later get() would loop forever).
 * Without a switch the resizes are synchronized, as the HashMap
 * documentation requires of concurrent structural changes.
 *
 * BUG_UNSYNCHRONIZED: the resizes run concurrently, the well-known infinite
 * loop of Java 7's HashMap. T1 reads A and its next B and stalls; T2 moves
 * the bucket, leaving B -> A; T1 then puts A, then B (whose next is now A),
 * then A again with A.next = B: a cycle A -> B -> A. Fails under every
 * model.
 */

struct entry {
	_Atomic(struct entry *) next;
};

struct entry a, b;
_Atomic(struct entry *) old_bucket;
_Atomic(struct entry *) new_bucket[2];	/* one new table per thread */
lock_t map_lock;

static void transfer(_Atomic(struct entry *) *dst)
{
	struct entry *e = atomic_load_explicit(&old_bucket, memory_order_relaxed);

	if (e == NULL)
		return;
	atomic_store_explicit(&old_bucket, NULL, memory_order_relaxed);
	do {
		struct entry *next = atomic_load_explicit(&e->next, memory_order_relaxed);

		atomic_store_explicit(&e->next, atomic_load_explicit(dst, memory_order_relaxed),
				      memory_order_relaxed);
		atomic_store_explicit(dst, e, memory_order_relaxed);
		e = next;
	} while (e != NULL);
}

static void *resizer(void *arg)
{
	_Atomic(struct entry *) *dst = &new_bucket[(long) arg];

#ifndef BUG_UNSYNCHRONIZED
	lock_acquire(&map_lock);
#endif
	transfer(dst);
#ifndef BUG_UNSYNCHRONIZED
	lock_release(&map_lock);
#endif
	return NULL;
}

/* Does the bucket hold a cycle? (it holds at most two entries otherwise) */
static bool has_cycle(_Atomic(struct entry *) *bucket)
{
	struct entry *e = atomic_load_explicit(bucket, memory_order_relaxed);

	for (int steps = 0; e != NULL; steps++) {
		if (steps > 2)
			return true;
		e = atomic_load_explicit(&e->next, memory_order_relaxed);
	}
	return false;
}

int main()
{
	pthread_t t1, t2;

	atomic_init(&b.next, NULL);
	atomic_init(&a.next, &b);
	atomic_init(&old_bucket, &a);

	if (pthread_create(&t1, NULL, resizer, (void *) 0L))
		abort();
	if (pthread_create(&t2, NULL, resizer, (void *) 1L))
		abort();
	if (pthread_join(t1, NULL))
		abort();
	if (pthread_join(t2, NULL))
		abort();
	assert(!has_cycle(&new_bucket[0]) && !has_cycle(&new_bucket[1]));
	return 0;
}
