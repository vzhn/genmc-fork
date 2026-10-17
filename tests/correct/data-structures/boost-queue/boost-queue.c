#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>

/*
 * Boost.Lockfree's queue (boost/lockfree/queue.hpp): a Michael-Scott queue
 * whose nodes come from a lock-free freelist (freelist.hpp), with tagged
 * node handles (an index and a tag in one word) against ABA. A node's next
 * field doubles as the freelist link, and the freelist keeps its tag
 * (set_ptr). When a node is taken from the freelist, the node constructor
 * sets next to null with the tag incremented, so a CAS that expected the
 * node's old next value fails. CASes are seq_cst, as boost::atomic's
 * default; the payload is atomic, since a pop may copy it from a node that
 * is being reused (it then discards the copy).
 *
 * Thread A pushes 10 and then pops; thread B pushes 2, pops and pushes 3;
 * main drains the queue. Every value comes out exactly once, and if A's pop
 * finds the queue empty, A's 10 must have been taken already, by B.
 *
 * BUG_TAG_RESET: Boost 1.71 to 1.73 (boostorg/lockfree#59). Commit 3d45c00
 * made the node constructor set next to (null, 0), resetting the tag of a
 * reused node. A, stalled in its push with the dummy node as tail and
 * (null, 0) as its next, links its node after that dummy once B has popped
 * it and is reusing it for 3, before B has linked it: A's push returns
 * with 10 unreachable, and A's pop finds the queue empty. The fix keeps
 * incrementing the tag.
 */

#define NNODES 5	/* index 0 is null; node 1 is the first dummy */
#define EMPTY -1

typedef uint64_t handle_t;	/* tag << 32 | index */

static handle_t make(unsigned idx, unsigned tag) { return (handle_t) tag << 32 | idx; }
static unsigned idx(handle_t h) { return (unsigned) h; }
static unsigned tag(handle_t h) { return (unsigned) (h >> 32); }

struct node {
	_Atomic(handle_t) next;
	atomic_int data;
};

struct node nodes[NNODES];
_Atomic(handle_t) head;
_Atomic(handle_t) tail;
_Atomic(handle_t) pool;		/* the freelist */

/* Freelist (a tagged Treiber stack); returns 0 if it is empty */
static unsigned pool_alloc(void)
{
	handle_t old = atomic_load_explicit(&pool, memory_order_acquire);

	for (;;) {
		if (idx(old) == 0)
			return 0;

		handle_t next = atomic_load_explicit(&nodes[idx(old)].next, memory_order_relaxed);

		if (atomic_compare_exchange_strong(&pool, &old, make(idx(next), tag(old) + 1)))
			return idx(old);
	}
}

static void pool_free(unsigned n)
{
	handle_t old = atomic_load_explicit(&pool, memory_order_relaxed);

	for (;;) {
		/* link the node, keeping the tag of its next field */
		handle_t next = atomic_load_explicit(&nodes[n].next, memory_order_relaxed);

		atomic_store_explicit(&nodes[n].next, make(idx(old), tag(next)), memory_order_relaxed);
		if (atomic_compare_exchange_strong(&pool, &old, make(n, tag(old))))
			return;
	}
}

static unsigned construct(int v)
{
	unsigned n = pool_alloc();

	assert(n != 0);	/* the pool is large enough */
	atomic_store_explicit(&nodes[n].data, v, memory_order_relaxed);
#ifdef BUG_TAG_RESET
	atomic_store_explicit(&nodes[n].next, make(0, 0), memory_order_release);
#else
	handle_t old = atomic_load_explicit(&nodes[n].next, memory_order_relaxed);

	atomic_store_explicit(&nodes[n].next, make(0, tag(old) + 1), memory_order_release);
#endif
	return n;
}

static void push(int v)
{
	unsigned n = construct(v);

	for (;;) {
		handle_t t = atomic_load_explicit(&tail, memory_order_acquire);
		handle_t next = atomic_load_explicit(&nodes[idx(t)].next, memory_order_acquire);
		handle_t t2 = atomic_load_explicit(&tail, memory_order_acquire);

		if (t != t2)
			continue;
		if (idx(next) == 0) {
			if (atomic_compare_exchange_strong(&nodes[idx(t)].next, &next,
							   make(n, tag(next) + 1))) {
				atomic_compare_exchange_strong(&tail, &t, make(n, tag(t) + 1));
				return;
			}
		} else {
			/* help: swing the tail forward */
			atomic_compare_exchange_strong(&tail, &t, make(idx(next), tag(t) + 1));
		}
	}
}

static int pop(void)
{
	for (;;) {
		handle_t h = atomic_load_explicit(&head, memory_order_acquire);
		handle_t t = atomic_load_explicit(&tail, memory_order_acquire);
		handle_t next = atomic_load_explicit(&nodes[idx(h)].next, memory_order_acquire);
		handle_t h2 = atomic_load_explicit(&head, memory_order_acquire);

		if (h != h2)
			continue;
		if (idx(h) == idx(t)) {
			if (idx(next) == 0)
				return EMPTY;
			atomic_compare_exchange_strong(&tail, &t, make(idx(next), tag(t) + 1));
		} else {
			if (idx(next) == 0)
				continue;

			int v = atomic_load_explicit(&nodes[idx(next)].data, memory_order_relaxed);

			if (atomic_compare_exchange_strong(&head, &h, make(idx(next), tag(h) + 1))) {
				pool_free(idx(h));
				return v;
			}
		}
	}
}

int a_pop = EMPTY;
int b_pop = EMPTY;
atomic_int seen[11];

static void got(int v)
{
	assert(v == 2 || v == 3 || v == 10);
	int before = atomic_fetch_add_explicit(&seen[v], 1, memory_order_relaxed);
	assert(before == 0);
}

static void *thread_a(void *unused)
{
	push(10);
	a_pop = pop();
	return NULL;
}

static void *thread_b(void *unused)
{
	push(2);
	b_pop = pop();
	push(3);
	return NULL;
}

int main()
{
	pthread_t a, b;
	int v;

	/* node 1 is the dummy, nodes 2.. are free */
	atomic_init(&head, make(1, 0));
	atomic_init(&tail, make(1, 0));
	for (unsigned n = NNODES - 1; n >= 2; n--)
		pool_free(n);

	if (pthread_create(&a, NULL, thread_a, NULL))
		abort();
	if (pthread_create(&b, NULL, thread_b, NULL))
		abort();
	if (pthread_join(a, NULL))
		abort();
	if (pthread_join(b, NULL))
		abort();

	/* a pop that follows A's push finds the queue empty only if 10 is gone */
	assert(a_pop != EMPTY || b_pop == 10);
	if (a_pop != EMPTY)
		got(a_pop);
	if (b_pop != EMPTY)
		got(b_pop);
	while ((v = pop()) != EMPTY)
		got(v);
	assert(seen[2] == 1 && seen[3] == 1 && seen[10] == 1);
	return 0;
}
