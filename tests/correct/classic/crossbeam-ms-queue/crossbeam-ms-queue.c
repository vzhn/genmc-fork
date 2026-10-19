#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>

/*
 * The blocking Michael-Scott queue of crossbeam 0.2 (src/sync/ms_queue.rs),
 * a dual queue: besides data nodes, the list can hold "blocked" nodes, each
 * a request from a consumer that found the queue empty. All non-sentinel
 * nodes are of one kind.
 *
 * - pop() first tries to take a data node. If there is none, it pushes a
 *   blocked node pointing to a Signal on its stack (here on the heap) and
 *   parks until the signal's ready flag is set, then returns the signal's
 *   data.
 * - push() appends a data node when the queue holds data or is empty.
 *   Otherwise it dequeues the first blocked node, writes the value into
 *   that node's signal, sets ready and unparks the consumer.
 *
 * thread::park() may return spuriously, so the consumer's wait is a loop on
 * the flag; here it is an await. Nodes are never freed (crossbeam frees
 * them through epochs). pop() loads the tail relaxed and reads the node
 * through it, relying on the address dependency; the model loads it with
 * acquire.
 *
 * Producers push 1..NPROD; one consumer pops NPROD values and checks that
 * it gets each value once.
 *
 * BUG_RELAXED_READY: the ready flag was stored and loaded relaxed, as before
 * commit bf1fb5627d67 ("Fixes ordering bug in ms queue", crossbeam#98,
 * 2017). The code relied on park()/unpark() for ordering, which they do not
 * give when park() returns spuriously: the consumer can see ready and read
 * the signal's data while the producer's write to it is unordered with the
 * read.
 */

#ifndef NPROD
# define NPROD 1
#endif

#ifdef BUG_RELAXED_READY
# define READY_STORE memory_order_relaxed
# define READY_LOAD memory_order_relaxed
#else
# define READY_STORE memory_order_release
# define READY_LOAD memory_order_acquire
#endif

struct signal {
	int data;
	atomic_bool ready;
};

struct node {
	bool is_data;
	int data;
	struct signal *signal;
	_Atomic(struct node *) next;
};

_Atomic(struct node *) head;
_Atomic(struct node *) tail;

static struct node *new_node(bool is_data, int data, struct signal *signal)
{
	struct node *n = malloc(sizeof(*n));

	n->is_data = is_data;
	n->data = data;
	n->signal = signal;
	atomic_init(&n->next, NULL);
	return n;
}

static bool cas(_Atomic(struct node *) *p, struct node *old, struct node *new,
		memory_order mo)
{
	return atomic_compare_exchange_strong_explicit(p, &old, new, mo, memory_order_relaxed);
}

/* Ok: linked n after onto; otherwise onto was not the tail */
static bool push_internal(struct node *onto, struct node *n)
{
	struct node *next = atomic_load_explicit(&onto->next, memory_order_acquire);

	if (next) {
		cas(&tail, onto, next, memory_order_release);
		return false;
	}
	if (!cas(&onto->next, NULL, n, memory_order_release))
		return false;
	cas(&tail, onto, n, memory_order_release);
	return true;
}

void push(int t)
{
	struct node *n = NULL;

	for (;;) {
		struct node *tl = atomic_load_explicit(&tail, memory_order_acquire);

		if (tl->is_data || atomic_load_explicit(&head, memory_order_relaxed) == tl) {
			if (!n)
				n = new_node(true, t, NULL);
			if (push_internal(tl, n))
				return;
			continue;
		}
		/* blocking mode: try to hand the value to a blocked consumer */
		struct node *hd = atomic_load_explicit(&head, memory_order_acquire);
		struct node *next = atomic_load_explicit(&hd->next, memory_order_acquire);

		if (next && !next->is_data && cas(&head, hd, next, memory_order_release)) {
			struct signal *s = next->signal;

			s->data = n ? n->data : t;
			atomic_store_explicit(&s->ready, true, READY_STORE);
			/* unpark */
			return;
		}
	}
}

/* 1: took *out; 0: no data; -1: lost a race */
static int pop_internal(int *out)
{
	struct node *hd = atomic_load_explicit(&head, memory_order_acquire);
	struct node *next = atomic_load_explicit(&hd->next, memory_order_acquire);

	if (!next || !next->is_data)
		return 0;
	if (!cas(&head, hd, next, memory_order_release))
		return -1;
	*out = next->data;
	return 1;
}

int pop(void)
{
	int r, res;

	while ((res = pop_internal(&r)) != 0)
		if (res == 1)
			return r;

	struct signal *s = malloc(sizeof(*s));
	s->data = 0;
	atomic_init(&s->ready, false);
	struct node *n = new_node(false, 0, s);

	for (;;) {
		if (pop_internal(&r) == 1)
			return r;

		/* Relaxed in crossbeam; the read through it relies on the address
		 * dependency, which C11 can only express as acquire. */
		struct node *tl = atomic_load_explicit(&tail, memory_order_acquire);
		if (tl->is_data &&
		    tl != atomic_load_explicit(&head, memory_order_relaxed))
			continue;

		if (push_internal(tl, n)) {
			/* while !ready { park() }: park() may return spuriously */
			__VERIFIER_await(atomic_load_explicit(&s->ready, READY_LOAD));
			return s->data;
		}
	}
}

void *producer(void *arg)
{
	push((int)(long) arg);
	return NULL;
}

void *consumer(void *arg)
{
	int seen = 0;

	for (int i = 0; i < NPROD; i++) {
		int v = pop();
		assert(v >= 1 && v <= NPROD);
		assert(!(seen & (1 << v)));
		seen |= 1 << v;
	}
	return NULL;
}

int main()
{
	pthread_t p[NPROD], c;
	struct node *sentinel = new_node(true, 0, NULL);

	atomic_init(&head, sentinel);
	atomic_init(&tail, sentinel);

	pthread_create(&c, NULL, consumer, NULL);
	for (int i = 0; i < NPROD; i++)
		pthread_create(&p[i], NULL, producer, (void *)(long)(i + 1));

	return 0;
}
