#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <assert.h>
#include <lkmm.h>

/*
 * The Linux kernel's lock-less list (include/linux/llist.h, lib/llist.c),
 * written with the kernel's primitives (<lkmm.h>);
 * tests/correct/data-structures/llist is the same model in C11 atomics. A
 * stack of nodes: llist_add() pushes with a cmpxchg loop, llist_del_all()
 * takes the whole list with an xchg, and llist_del_first() pops the first
 * node with a cmpxchg from first to first->next.
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
 *
 * Under <lkmm.h>'s C11 mapping (-imm) the correct version fails too, and
 * that is a gap of the mapping, not of llist: when C1's cmpxchg fails
 * because C2 added A back, C1 re-reads A->next through an address
 * dependency on the value the failed cmpxchg returned. LKMM orders that
 * read after the read of first (addr in to-r) and after C2's write of
 * A->next (wr-vis), so C1 sees A->next = C; IMM orders a dependency into a
 * write only, and a failed cmpxchg carries no acquire, so C1 may read the
 * stale A->next = B and pop A with first = B, losing C. The test waits for
 * -lkmm (tests/cooking/lkmm).
 */

struct llist_node {
	struct llist_node *next;
};

struct llist_head {
	struct llist_node *first;
};

struct llist_head list;
struct llist_node a, b, c;
DEFINE_SPINLOCK(del_lock);
int outs[3];		/* how often a, b, c came out */

static void llist_add(struct llist_node *new, struct llist_head *head)
{
	struct llist_node *first = READ_ONCE(head->first);

	do {
		/* llist.h writes this plainly; under the C11 mapping a plain write
		 * published by a cmpxchg and read through an address dependency is a
		 * data race (LKMM's plain-access rules order it) */
		WRITE_ONCE(new->next, first);
	} while (!try_cmpxchg(&head->first, &first, new));
}

static struct llist_node *llist_del_all(struct llist_head *head)
{
	return xchg(&head->first, NULL);
}

static struct llist_node *llist_del_first(struct llist_head *head)
{
	struct llist_node *entry, *next;

#ifndef BUG_NO_LOCK
	spin_lock(&del_lock);
#endif
	entry = smp_load_acquire(&head->first);
	do {
		if (entry == NULL)
			break;
		next = READ_ONCE(entry->next);
	} while (!try_cmpxchg(&head->first, &entry, next));
#ifndef BUG_NO_LOCK
	spin_unlock(&del_lock);
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
	got1 = llist_del_first(&list);
	return NULL;
}

static void *consumer_2(void *unused)
{
	got2 = llist_del_first(&list);
	llist_add(&c, &list);
	if (got2)
		llist_add(got2, &list);
	return NULL;
}

int main()
{
	pthread_t c1, c2;

	b.next = NULL;
	a.next = &b;
	list.first = &a;

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
	for (struct llist_node *n = llist_del_all(&list); n; n = n->next)
		count(n);
	for (int i = 0; i < 3; i++)
		assert(outs[i] == ins[i]);
	return 0;
}
