#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>

/*
 * The lock-free stack with split reference counts from Anthony Williams,
 * C++ Concurrency in Action, ch. 7 (Listing 7.13, with memory orders).
 * head is a counted pointer: a node pointer and an external count, in one
 * word (the count in the top 16 bits). A pop first increments the external
 * count of the head it read, so the node cannot be freed under it, then
 * tries to swing head to the next node. The winner takes the data and adds
 * external - 2 to the node's internal count; a loser subtracts one. Whoever
 * brings the internal count to zero frees the node.
 *
 * The internal count is updated as reference counts usually are: every
 * update is a release, and the thread that brings it to zero acquires
 * before freeing (the winner's update is acq_rel; a losing pop that frees
 * does an acquire load first, as in the book).
 *
 * main pushes 1 and 2; threads P1 and P2 pop, and with PUSHER thread P3
 * pushes 3 meanwhile. Every value is popped exactly once (main pops the
 * rest) and every node is freed exactly once. Two pops can keep failing
 * each other's CAS by incrementing the count, without end, so the tests
 * bound the loops (-unroll=3).
 *
 * One switch per bug:
 * - BUG_AS_PRINTED: the orderings of Listing 7.13, where a losing pop
 *   decrements relaxed and the winner adds with release only. When the
 *   winner frees the node, the loser's accesses to it (its read of next and
 *   its decrement) are not ordered before the free;
 * - BUG_NO_ACQUIRE: a losing pop that brings the count to zero frees the
 *   node without the acquire load, so the free is not ordered after the
 *   winner's read of the data.
 */

#define PTR_MASK ((1ULL << 48) - 1)

#ifdef BUG_AS_PRINTED
# define WINNER_ADD memory_order_release
# define LOSER_SUB memory_order_relaxed
#else
# define WINNER_ADD memory_order_acq_rel
# define LOSER_SUB memory_order_release
#endif

typedef uint64_t counted_t;

struct node {
	int data;
	atomic_int internal_count;
	counted_t next;		/* set before the node is published */
};

_Atomic(counted_t) head;
atomic_int popped[4];

static counted_t make(struct node *p, unsigned count)
{
	return (uint64_t) (uintptr_t) p | (uint64_t) count << 48;
}

static struct node *ptr_of(counted_t c) { return (struct node *) (uintptr_t) (c & PTR_MASK); }
static unsigned count_of(counted_t c) { return c >> 48; }

static void push(int v)
{
	struct node *n = malloc(sizeof(*n));
	counted_t new;

	n->data = v;
	atomic_init(&n->internal_count, 0);
	new = make(n, 1);
	n->next = atomic_load_explicit(&head, memory_order_relaxed);
	while (!atomic_compare_exchange_strong_explicit(&head, &n->next, new, memory_order_release,
							memory_order_relaxed))
		;
}

static void increase_head_count(counted_t *old)
{
	counted_t new;

	do {
		new = make(ptr_of(*old), count_of(*old) + 1);
	} while (!atomic_compare_exchange_strong_explicit(&head, old, new, memory_order_acquire,
							   memory_order_relaxed));
	*old = new;
}

static bool pop(int *v)
{
	counted_t old = atomic_load_explicit(&head, memory_order_relaxed);

	for (;;) {
		increase_head_count(&old);

		struct node *p = ptr_of(old);

		if (p == NULL)
			return false;
		if (atomic_compare_exchange_strong_explicit(&head, &old, p->next,
							    memory_order_relaxed,
							    memory_order_relaxed)) {
			*v = p->data;

			int increase = count_of(old) - 2;

			if (atomic_fetch_add_explicit(&p->internal_count, increase, WINNER_ADD) ==
			    -increase)
				free(p);
			return true;
		}
		if (atomic_fetch_add_explicit(&p->internal_count, -1, LOSER_SUB) == 1) {
#ifndef BUG_NO_ACQUIRE
			atomic_load_explicit(&p->internal_count, memory_order_acquire);
#endif
			free(p);
		}
	}
}

static void got(int v)
{
	assert(v >= 1 && v <= 3);
	int before = atomic_fetch_add_explicit(&popped[v], 1, memory_order_relaxed);
	assert(before == 0);
}

static void *popper(void *unused)
{
	int v;

	if (pop(&v))
		got(v);
	return NULL;
}

static void *pusher(void *unused)
{
	push(3);
	return NULL;
}

int main()
{
	pthread_t p1, p2, p3;
	int v;

	push(1);
	push(2);
	if (pthread_create(&p1, NULL, popper, NULL))
		abort();
	if (pthread_create(&p2, NULL, popper, NULL))
		abort();
#ifdef PUSHER
	if (pthread_create(&p3, NULL, pusher, NULL))
		abort();
	if (pthread_join(p3, NULL))
		abort();
#endif
	if (pthread_join(p1, NULL))
		abort();
	if (pthread_join(p2, NULL))
		abort();

	while (pop(&v))
		got(v);
	assert(popped[1] == 1 && popped[2] == 1);
#ifdef PUSHER
	assert(popped[3] == 1);
#endif
	return 0;
}
