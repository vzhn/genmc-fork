#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>

/*
 * The Snark deque (Detlefs, Flood, Garthwaite, Martin, Shavit, Steele, Even
 * Better DCAS-Based Concurrent Deques, DISC 2000): a doubly linked list
 * between sentinels, with LeftHat and RightHat pointing to the outermost
 * nodes, built on DCAS (a CAS on two locations at once). A node's outward
 * self pointer marks a sentinel; Dummy is the initial sentinel. Code as
 * reproduced in Doherty et al., DCAS is not a Silver Bullet for
 * Nonblocking Algorithm Design (SPAA 2004), Figs. 2-4 and 8; the left-hand
 * operations mirror the right-hand ones.
 *
 * Each shared access and each DCAS is one atomic step: all of them run
 * under one global mutex, so a DCAS is atomic with respect to every read,
 * as in the paper's model. Nodes are never freed (the paper assumes GC).
 *
 * Without a switch the pops are the corrected ones (Fig. 8): no special
 * case for one element, "empty" only if the hat still points to the
 * sentinel, and a CAS that claims the node's value, so a node removed twice
 * yields its value once.
 *
 * Three clients; the last two are the scenarios of the two bugs of the
 * original pops (SNARK_ORIGINAL, Fig. 4):
 * - by default, the deque holds [B]; p pops right while q pops left. B
 *   comes out once;
 * - EMPTY_CLIENT: the deque holds [B]; p pops right while q pushes C on the
 *   right and pops left. The deque is never empty, so p must not return
 *   empty. In the original, p reads RightHat = B, q's pops remove B (which
 *   gets its self pointer) and p takes B's self pointer for an empty deque;
 * - TWICE: the deque holds [A, B]; p pops right while t2 pops left, pushes
 *   C and pops left again and t3 pops right. Every value must come out
 *   once. In the original, p is about to DCAS RightHat and B->L when B is
 *   removed by t2's popLeft and t3's popRight moves RightHat back onto B;
 *   p's DCAS then succeeds and returns B's value a second time (the ABA
 *   problem, extended to two locations; t2's second popLeft must stop
 *   between its DCAS and its store of Dummy into B->L). To keep the search
 *   small, every operation runs as one atomic step that can only be
 *   preempted just before and just after a pop's removing DCAS: a subset
 *   of the interleavings, so what it finds is a real execution.
 */

#define EMPTY 0
#define CLAIMED -1

struct node {
	int V;
	struct node *L, *R;
};

pthread_mutex_t step = PTHREAD_MUTEX_INITIALIZER;	/* each shared access is atomic */

#ifdef TWICE
/* whole operations as one step, preemptible only around a pop's DCAS */
__thread bool in_block;
# define step_begin() do { if (!in_block) pthread_mutex_lock(&step); } while (0)
# define step_end() do { if (!in_block) pthread_mutex_unlock(&step); } while (0)
# define preempt()							\
	do {								\
		if (in_block) {						\
			pthread_mutex_unlock(&step);			\
			pthread_mutex_lock(&step);			\
		}							\
	} while (0)
# define BLOCK(op)							\
	do {								\
		pthread_mutex_lock(&step);				\
		in_block = true;					\
		op;							\
		in_block = false;					\
		pthread_mutex_unlock(&step);				\
	} while (0)
#else
# define step_begin() pthread_mutex_lock(&step)
# define step_end() pthread_mutex_unlock(&step)
# define preempt() do {} while (0)
#endif

struct node Dummy = { 0, &Dummy, &Dummy };
struct node *LeftHat = &Dummy, *RightHat = &Dummy;

static struct node *ld(struct node **p)
{
	step_begin();
	struct node *v = *p;
	step_end();
	return v;
}

static int ldv(struct node *n)
{
	step_begin();
	int v = n->V;
	step_end();
	return v;
}

static void st(struct node **p, struct node *v)
{
	step_begin();
	*p = v;
	step_end();
}

static bool dcas(struct node **a1, struct node **a2, struct node *o1, struct node *o2,
		 struct node *n1, struct node *n2)
{
	bool ok;

	step_begin();
	ok = *a1 == o1 && *a2 == o2;
	if (ok) {
		*a1 = n1;
		*a2 = n2;
	}
	step_end();
	return ok;
}

static bool cas_v(struct node *n, int old, int new)
{
	bool ok;

	step_begin();
	ok = n->V == old;
	if (ok)
		n->V = new;
	step_end();
	return ok;
}

static struct node *new_node(int v)
{
	struct node *nd = malloc(sizeof(*nd));

	nd->V = v;
	return nd;
}

static void push_right(int v)
{
	struct node *nd = new_node(v);

	nd->R = &Dummy;
	for (;;) {
		struct node *rh = ld(&RightHat);
		struct node *rhR = ld(&rh->R);

		if (rhR == rh) {
			nd->L = &Dummy;
			struct node *lh = ld(&LeftHat);
			if (dcas(&RightHat, &LeftHat, rh, lh, nd, nd))
				return;
		} else {
			nd->L = rh;
			if (dcas(&RightHat, &rh->R, rh, rhR, nd, nd))
				return;
		}
	}
}

#ifdef SNARK_ORIGINAL
static int pop_right(void)
{
	for (;;) {
		struct node *rh = ld(&RightHat);
		struct node *lh = ld(&LeftHat);

		if (ld(&rh->R) == rh)
			return EMPTY;
		if (rh == lh) {
			if (dcas(&RightHat, &LeftHat, rh, lh, &Dummy, &Dummy))
				return ldv(rh);
		} else {
			struct node *rhL = ld(&rh->L);

			preempt();
			if (dcas(&RightHat, &rh->L, rh, rhL, rhL, rh)) {
				int result = ldv(rh);

				preempt();
				st(&rh->R, &Dummy);
				return result;
			}
		}
	}
}

static int pop_left(void)
{
	for (;;) {
		struct node *lh = ld(&LeftHat);
		struct node *rh = ld(&RightHat);

		if (ld(&lh->L) == lh)
			return EMPTY;
		if (lh == rh) {
			if (dcas(&LeftHat, &RightHat, lh, rh, &Dummy, &Dummy))
				return ldv(lh);
		} else {
			struct node *lhR = ld(&lh->R);

			preempt();
			if (dcas(&LeftHat, &lh->R, lh, lhR, lhR, lh)) {
				int result = ldv(lh);

				preempt();
				st(&lh->L, &Dummy);
				return result;
			}
		}
	}
}
#else
static int pop_right(void)
{
	for (;;) {
		struct node *rh = ld(&RightHat);
		struct node *rhL = ld(&rh->L);

		if (ld(&rh->R) == rh) {
			if (ld(&RightHat) == rh)
				return EMPTY;
			continue;
		}
		preempt();
		if (dcas(&RightHat, &rh->L, rh, rhL, rhL, rh)) {
			int result = ldv(rh);

			if (result != CLAIMED && cas_v(rh, result, CLAIMED)) {
				preempt();
				st(&rh->R, &Dummy);
				return result;
			}
			return EMPTY;
		}
	}
}

static int pop_left(void)
{
	for (;;) {
		struct node *lh = ld(&LeftHat);
		struct node *lhR = ld(&lh->R);

		if (ld(&lh->L) == lh) {
			if (ld(&LeftHat) == lh)
				return EMPTY;
			continue;
		}
		preempt();
		if (dcas(&LeftHat, &lh->R, lh, lhR, lhR, lh)) {
			int result = ldv(lh);

			if (result != CLAIMED && cas_v(lh, result, CLAIMED)) {
				preempt();
				st(&lh->L, &Dummy);
				return result;
			}
			return EMPTY;
		}
	}
}
#endif

#define A 1
#define B 2
#define C 3

int seen[4];
int p_result, q_result, t2_first, t2_second, t3_result;

static void got(int v)
{
	if (v != EMPTY) {
		assert(v >= A && v <= C);
		seen[v]++;
	}
}

static void *thread_p(void *unused)
{
#ifdef TWICE
	BLOCK(p_result = pop_right());
#else
	p_result = pop_right();
#endif
	return NULL;
}

#if defined(EMPTY_CLIENT)
static void *thread_q(void *unused)
{
	push_right(C);
	q_result = pop_left();
	return NULL;
}
#elif defined(TWICE)
static void *thread_q(void *unused)
{
	BLOCK(t2_first = pop_left());
	BLOCK(push_right(C));
	BLOCK(t2_second = pop_left());
	return NULL;
}

static void *thread_t3(void *unused)
{
	BLOCK(t3_result = pop_right());
	return NULL;
}
#else
static void *thread_q(void *unused)
{
	q_result = pop_left();
	return NULL;
}
#endif

int main()
{
	pthread_t p, q;
	int v;

#ifdef TWICE
	pthread_t t3;

	push_right(A);
#endif
	push_right(B);
	if (pthread_create(&p, NULL, thread_p, NULL))
		abort();
	if (pthread_create(&q, NULL, thread_q, NULL))
		abort();
#ifdef TWICE
	if (pthread_create(&t3, NULL, thread_t3, NULL))
		abort();
	if (pthread_join(t3, NULL))
		abort();
#endif
	if (pthread_join(p, NULL))
		abort();
	if (pthread_join(q, NULL))
		abort();

#ifdef EMPTY_CLIENT
	/* the deque held an element throughout p's pop */
	assert(p_result != EMPTY);
#endif
	got(p_result);
	got(q_result);
	got(t2_first);
	got(t2_second);
	got(t3_result);
	while ((v = pop_left()) != EMPTY)
		got(v);
#if defined(TWICE)
	assert(seen[A] == 1 && seen[B] == 1 && seen[C] == 1);
#elif defined(EMPTY_CLIENT)
	assert(seen[B] == 1 && seen[C] == 1);
#else
	assert(seen[B] == 1);
#endif
	return 0;
}
