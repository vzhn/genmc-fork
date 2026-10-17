#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>

/*
 * The Harris-Michael lock-free sorted list (Harris, A Pragmatic
 * Implementation of Non-Blocking Linked-Lists, DISC 2001; Michael, High
 * Performance Dynamic Lock-Free Hash Tables and List-Based Sets, SPAA
 * 2002), used as a set. A node's next pointer carries a mark in its low
 * bit. delete() first marks the node's own next pointer (logical
 * deletion), so no node can be linked after it any more, and then unlinks
 * it; find() unlinks marked nodes it passes. Nodes are not reclaimed while
 * the threads run (main frees them at the end).
 *
 * The set starts as {1, 3}. Thread T1 deletes 1 and thread T2 inserts 2;
 * with THIRD, thread T3 deletes 3, the node 2 is linked before. Afterwards
 * the set holds exactly the keys the operations say it should.
 *
 * BUG_NO_MARK: delete() unlinks the node with a CAS on its predecessor
 * without marking it first, the error Harris's paper starts from. T2 links
 * 2 after 1 while T1 unlinks 1 with the next pointer it read before: 2 is
 * lost although its insert succeeded.
 */

#define MAXNODES 4

struct node {
	int key;
	_Atomic(uintptr_t) next;	/* struct node *, low bit: marked */
};

_Atomic(uintptr_t) head;		/* points to the first node */
struct node *allocated[MAXNODES];
atomic_int nallocated;

static struct node *ptr(uintptr_t p) { return (struct node *) (p & ~(uintptr_t) 1); }
static bool marked(uintptr_t p) { return p & 1; }

static struct node *new_node(int key, uintptr_t next)
{
	struct node *n = malloc(sizeof(*n));

	n->key = key;
	atomic_init(&n->next, next);
	allocated[atomic_fetch_add_explicit(&nallocated, 1, memory_order_relaxed)] = n;
	return n;
}

/*
 * Finds the first node with a key >= key. Sets *prev to the link that
 * points to it and *curr to it (or 0). Unlinks marked nodes on the way.
 */
static bool find(int key, _Atomic(uintptr_t) **prev, uintptr_t *curr)
{
again:
	*prev = &head;
	*curr = atomic_load_explicit(*prev, memory_order_acquire);
	for (;;) {
		if (*curr == 0)
			return false;

		struct node *c = ptr(*curr);
		uintptr_t next = atomic_load_explicit(&c->next, memory_order_acquire);

		if (atomic_load_explicit(*prev, memory_order_acquire) != *curr)
			goto again;
		if (!marked(next)) {
			if (c->key >= key)
				return c->key == key;
			*prev = &c->next;
		} else {
			uintptr_t expected = *curr;

			/* unlink the deleted node */
			if (!atomic_compare_exchange_strong_explicit(*prev, &expected,
								     (uintptr_t) ptr(next),
								     memory_order_acq_rel,
								     memory_order_acquire))
				goto again;
		}
		*curr = (uintptr_t) ptr(next);
	}
}

static bool insert(int key)
{
	_Atomic(uintptr_t) *prev;
	uintptr_t curr;
	struct node *n = NULL;

	for (;;) {
		if (find(key, &prev, &curr))
			return false;
		if (n == NULL)
			n = new_node(key, curr);
		else
			atomic_store_explicit(&n->next, curr, memory_order_relaxed);
		if (atomic_compare_exchange_strong_explicit(prev, &curr, (uintptr_t) n,
							    memory_order_acq_rel,
							    memory_order_acquire))
			return true;
	}
}

static bool delete(int key)
{
	_Atomic(uintptr_t) *prev;
	uintptr_t curr;

	for (;;) {
		if (!find(key, &prev, &curr))
			return false;

		struct node *c = ptr(curr);
		uintptr_t next = atomic_load_explicit(&c->next, memory_order_acquire);

		if (marked(next))
			continue;
#ifndef BUG_NO_MARK
		/* logical deletion */
		if (!atomic_compare_exchange_strong_explicit(&c->next, &next, next | 1,
							     memory_order_acq_rel,
							     memory_order_acquire))
			continue;
		/* physical deletion; find() finishes it if this fails */
		if (!atomic_compare_exchange_strong_explicit(prev, &curr, next,
							     memory_order_acq_rel,
							     memory_order_acquire))
			find(key, &prev, &curr);
		return true;
#else
		if (atomic_compare_exchange_strong_explicit(prev, &curr, next,
							    memory_order_acq_rel,
							    memory_order_acquire))
			return true;
#endif
	}
}

static bool contains(int key)
{
	_Atomic(uintptr_t) *prev;
	uintptr_t curr;

	return find(key, &prev, &curr);
}

bool deleted, inserted, deleted3;

static void *thread_1(void *unused)
{
	deleted = delete(1);
	return NULL;
}

static void *thread_2(void *unused)
{
	inserted = insert(2);
	return NULL;
}

static void *thread_3(void *unused)
{
	deleted3 = delete(3);
	return NULL;
}

int main()
{
	pthread_t t1, t2, t3;
	struct node *n3 = new_node(3, 0);
	struct node *n1 = new_node(1, (uintptr_t) n3);

	atomic_init(&head, (uintptr_t) n1);

	if (pthread_create(&t1, NULL, thread_1, NULL))
		abort();
	if (pthread_create(&t2, NULL, thread_2, NULL))
		abort();
#ifdef THIRD
	if (pthread_create(&t3, NULL, thread_3, NULL))
		abort();
	if (pthread_join(t3, NULL))
		abort();
	assert(deleted3);
#endif
	if (pthread_join(t1, NULL))
		abort();
	if (pthread_join(t2, NULL))
		abort();

	assert(deleted && inserted);
	assert(!contains(1));
	assert(contains(2));
#ifdef THIRD
	assert(!contains(3));
#else
	assert(contains(3));
#endif
	for (int i = 0; i < nallocated; i++)
		free(allocated[i]);
	return 0;
}
