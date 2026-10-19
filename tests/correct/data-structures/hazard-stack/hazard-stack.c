#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>

/*
 * A Treiber stack with hazard pointers written by hand (Michael, Hazard
 * Pointers: Safe Memory Reclamation for Lock-Free Objects, IEEE TPDS 2004),
 * rather than with GenMC's __VERIFIER_hp_* API. Before reading a node, a
 * pop publishes it in its hazard pointer, issues a store-load fence and
 * checks that the node is still on top; only then may it dereference it.
 * A pop that removed a node retires it and scans: after a fence, it frees
 * each retired node that no hazard pointer protects and keeps the others.
 * In C11 every update of a hazard pointer is a release and the scan reads
 * them with acquire, so that a thread's accesses to a node it protected
 * happen before the node is freed (Michael's paper assumes SC).
 *
 * main pushes 1 and 2; threads P1 and P2 pop once each; main then pops the
 * rest and frees what is still retired. Every value comes out once and no
 * freed node is touched.
 *
 * One switch per bug (the first two skip steps Michael's paper insists on):
 * - BUG_NO_VALIDATE: the pop does not re-check top after publishing the
 *   hazard pointer. P1 reads top = node 2 and stalls; P2 pops node 2,
 *   retires it and, seeing no hazard, frees it; P1 then protects a freed
 *   node and reads its next field. Fails under every model;
 * - BUG_NO_FENCE: the re-check is there but without the fence. P1's store
 *   of its hazard pointer and P2's scan of it form store buffering with
 *   the update of top: P1 sees the old top while P2 sees no hazard. Passes
 *   under SC;
 * - BUG_RELAXED_HP: hazard pointers are updated and scanned with relaxed
 *   accesses. P1 reads a node's next while protecting it and then moves its
 *   hazard pointer on; P2's scan sees the new value and frees the node, but
 *   P1's read is not ordered before the free. Passes under SC.
 */

#define NTHREADS 2
#define MAX_RETIRED 2

#ifdef BUG_RELAXED_HP
# define HP_STORE memory_order_relaxed
# define HP_LOAD memory_order_relaxed
#else
# define HP_STORE memory_order_release
# define HP_LOAD memory_order_acquire
#endif

struct node {
	int value;
	struct node *next;	/* set before the node is published */
};

_Atomic(struct node *) top;
_Atomic(struct node *) hp[NTHREADS];
struct node *retired[NTHREADS][MAX_RETIRED];
int nretired[NTHREADS];
atomic_int popped[3];

static void push(int v)
{
	struct node *n = malloc(sizeof(*n));
	struct node *t = atomic_load_explicit(&top, memory_order_relaxed);

	n->value = v;
	do {
		n->next = t;
	} while (!atomic_compare_exchange_strong_explicit(&top, &t, n, memory_order_release,
							   memory_order_relaxed));
}

static void scan(int me)
{
	int kept = 0;

	atomic_thread_fence(memory_order_seq_cst);
	for (int i = 0; i < nretired[me]; i++) {
		struct node *n = retired[me][i];
		bool hazard = false;

		for (int j = 0; j < NTHREADS; j++)
			if (atomic_load_explicit(&hp[j], HP_LOAD) == n)
				hazard = true;
		if (hazard)
			retired[me][kept++] = n;
		else
			free(n);
	}
	nretired[me] = kept;
}

static void retire(int me, struct node *n)
{
	retired[me][nretired[me]++] = n;
	scan(me);
}

/* Returns the value popped, or 0 if the stack is empty */
static int pop(int me)
{
	for (;;) {
		struct node *t = atomic_load_explicit(&top, memory_order_acquire);

		if (t == NULL)
			return 0;
		atomic_store_explicit(&hp[me], t, HP_STORE);
#ifndef BUG_NO_FENCE
		atomic_thread_fence(memory_order_seq_cst);
#endif
#ifndef BUG_NO_VALIDATE
		if (atomic_load_explicit(&top, memory_order_acquire) != t)
			continue;
#endif
		struct node *next = t->next;

		if (atomic_compare_exchange_strong_explicit(&top, &t, next, memory_order_acq_rel,
							    memory_order_relaxed)) {
			int v = t->value;

			atomic_store_explicit(&hp[me], NULL, HP_STORE);
			retire(me, t);
			return v;
		}
	}
}

static void got(int v)
{
	assert(v == 1 || v == 2);
	int before = atomic_fetch_add_explicit(&popped[v], 1, memory_order_relaxed);
	assert(before == 0);
}

static void *popper(void *arg)
{
	int v = pop((intptr_t) arg);

	if (v)
		got(v);
	return NULL;
}

int main()
{
	pthread_t p[NTHREADS];
	int v;

	push(1);
	push(2);
	for (intptr_t i = 0; i < NTHREADS; i++)
		if (pthread_create(&p[i], NULL, popper, (void *) i))
			abort();
	for (int i = 0; i < NTHREADS; i++)
		if (pthread_join(p[i], NULL))
			abort();

	while ((v = pop(0)))
		got(v);
	for (int i = 0; i < NTHREADS; i++)
		scan(i);
	assert(popped[1] == 1 && popped[2] == 1);
	return 0;
}
