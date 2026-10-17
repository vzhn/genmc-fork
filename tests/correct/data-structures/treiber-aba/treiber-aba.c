#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>

/*
 * A Treiber stack whose popped nodes are recycled through a freelist, the
 * setting of the ABA problem (IBM System/370 Principles of Operation; The
 * Art of Multiprocessor Programming, Fig. 10.14). top is a tagged
 * reference: a node index and a counter in one word, and every successful
 * CAS increments the counter, so a CAS that read an old top fails even if
 * the same node is back on top. Node fields are relaxed atomics: a pop
 * may read the next field of a node that is being recycled (and then fails
 * its CAS).
 *
 * The stack holds A(1) -> B(2). T1 pops once. T2 pops A and B, frees B and
 * then A, and pushes 3, which reuses A's node. main pops the rest. Every
 * value comes out exactly once.
 *
 * BUG_NO_TAG: top is a bare node index. T1 reads top = A and A's next = B
 * and stalls; T2 pops A and B and pushes 3 into the recycled A; T1's CAS
 * from A to B succeeds and puts the freed node B on top, whose value 2
 * then comes out a second time. Fails under every model.
 */

#define NNODES 4	/* index 0 is null */

struct node {
	atomic_int value;
	atomic_uint next;
};

struct node nodes[NNODES];
_Atomic(uint64_t) top;		/* tag << 32 | index */
unsigned int freelist[NNODES];	/* T2's, LIFO */
int nfree;
unsigned int next_fresh = 1;	/* main's allocations */
atomic_int seen[4];

static uint64_t make(unsigned int idx, unsigned int tag)
{
#ifdef BUG_NO_TAG
	tag = 0;
#endif
	return (uint64_t) tag << 32 | idx;
}
static unsigned int idx(uint64_t t) { return (unsigned int) t; }
static unsigned int tag(uint64_t t) { return (unsigned int) (t >> 32); }

static void push_node(unsigned int n, int v)
{
	uint64_t old = atomic_load_explicit(&top, memory_order_relaxed);

	atomic_store_explicit(&nodes[n].value, v, memory_order_relaxed);
	do {
		atomic_store_explicit(&nodes[n].next, idx(old), memory_order_relaxed);
	} while (!atomic_compare_exchange_strong_explicit(&top, &old, make(n, tag(old) + 1),
							   memory_order_release,
							   memory_order_relaxed));
}

/* Returns the popped node, or 0 if the stack is empty */
static unsigned int pop_node(void)
{
	uint64_t old = atomic_load_explicit(&top, memory_order_acquire);

	for (;;) {
		if (idx(old) == 0)
			return 0;

		unsigned int next = atomic_load_explicit(&nodes[idx(old)].next,
							 memory_order_relaxed);

		if (atomic_compare_exchange_strong_explicit(&top, &old, make(next, tag(old) + 1),
							    memory_order_acquire,
							    memory_order_acquire))
			return idx(old);
	}
}

static void got(unsigned int n)
{
	int v = atomic_load_explicit(&nodes[n].value, memory_order_relaxed);

	assert(v >= 1 && v <= 3);
	int before = atomic_fetch_add_explicit(&seen[v], 1, memory_order_relaxed);
	assert(before == 0);
}

static void *thread_1(void *unused)
{
	unsigned int n = pop_node();

	if (n)
		got(n);
	return NULL;
}

static void *thread_2(void *unused)
{
	unsigned int a = pop_node();
	unsigned int b = pop_node();

	if (a)
		got(a);
	if (b) {
		got(b);
		freelist[nfree++] = b;
	}
	if (a)
		freelist[nfree++] = a;
	if (nfree > 0)
		push_node(freelist[--nfree], 3);
	return NULL;
}

int main()
{
	pthread_t t1, t2;
	unsigned int n;

	push_node(next_fresh++, 2);	/* B */
	push_node(next_fresh++, 1);	/* A, on top */
	if (pthread_create(&t1, NULL, thread_1, NULL))
		abort();
	if (pthread_create(&t2, NULL, thread_2, NULL))
		abort();
	if (pthread_join(t1, NULL))
		abort();
	if (pthread_join(t2, NULL))
		abort();

	while ((n = pop_node()))
		got(n);
	return 0;
}
