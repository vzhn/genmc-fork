#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>
#include "../../../lib/lock.h"

/*
 * The Linux kernel's lock-less list (include/linux/llist.h, lib/llist.c): a
 * stack of nodes. llist_add() pushes with a cmpxchg loop, llist_del_all()
 * takes the whole list with an xchg, and llist_del_first() pops the first
 * node with a cmpxchg from first to first->next. Kernel primitives map to
 * C11: cmpxchg and xchg are seq_cst, smp_load_acquire is acquire and
 * READ_ONCE is relaxed.
 *
 * llist.h: llist_del_first() may run concurrently with llist_add() without
 * a lock, but two llist_del_first() need one, "because llist_del_first
 * depends on list->first->next not changing": "a llist_del_first,
 * llist_add, llist_add (or llist_del_all, llist_add, llist_add) sequence in
 * another consumer may cause violations".
 *
 * The list starts as A -> B. Consumer C1 deletes the first node; consumer
 * C2 deletes the first node, adds node C and adds back the node it
 * deleted. Without a switch, the llist_del_first() calls hold a lock. At
 * the end main takes the rest with llist_del_all(): every node added comes
 * out as often as it was added.
 *
 * BUG_NO_LOCK: the two llist_del_first() run without the lock. C1 reads
 * first = A and A->next = B; C2 deletes A, adds C and adds A back; C1's
 * cmpxchg finds A first again and sets first to B: C is lost.
 */

struct llist_node {
	_Atomic(struct llist_node *) next;
};

_Atomic(struct llist_node *) first;
struct llist_node a, b, c;
lock_t del_lock;
int outs[3];		/* how often a, b, c came out */

static void llist_add(struct llist_node *new)
{
	struct llist_node *f = atomic_load_explicit(&first, memory_order_relaxed);

	do {
		atomic_store_explicit(&new->next, f, memory_order_relaxed);
	} while (!atomic_compare_exchange_strong(&first, &f, new));
}

static struct llist_node *llist_del_all(void)
{
	return atomic_exchange(&first, NULL);
}

static struct llist_node *llist_del_first(void)
{
	struct llist_node *entry, *next;

#ifndef BUG_NO_LOCK
	lock_acquire(&del_lock);
#endif
	entry = atomic_load_explicit(&first, memory_order_acquire);
	do {
		if (entry == NULL)
			break;
		next = atomic_load_explicit(&entry->next, memory_order_relaxed);
	} while (!atomic_compare_exchange_strong(&first, &entry, next));
#ifndef BUG_NO_LOCK
	lock_release(&del_lock);
#endif
	return entry;
}

static void count(struct llist_node *n)
{
	outs[n == &a ? 0 : n == &b ? 1 : 2]++;
}

struct llist_node *got1, *got2;

static void *consumer_1(void *unused)
{
	got1 = llist_del_first();
	return NULL;
}

static void *consumer_2(void *unused)
{
	got2 = llist_del_first();
	llist_add(&c);
	if (got2)
		llist_add(got2);
	return NULL;
}

int main()
{
	pthread_t c1, c2;

	atomic_init(&b.next, NULL);
	atomic_init(&a.next, &b);
	atomic_init(&first, &a);

	if (pthread_create(&c1, NULL, consumer_1, NULL))
		abort();
	if (pthread_create(&c2, NULL, consumer_2, NULL))
		abort();
	if (pthread_join(c1, NULL))
		abort();
	if (pthread_join(c2, NULL))
		abort();

	/* adds: a, b, c, and got2 again */
	int ins[3] = { 1, 1, 1 };

	if (got2)
		ins[got2 == &a ? 0 : got2 == &b ? 1 : 2]++;
	if (got1)
		count(got1);
	if (got2)
		count(got2);
	for (struct llist_node *n = llist_del_all(); n;
	     n = atomic_load_explicit(&n->next, memory_order_relaxed))
		count(n);
	for (int i = 0; i < 3; i++)
		assert(outs[i] == ins[i]);
	return 0;
}
