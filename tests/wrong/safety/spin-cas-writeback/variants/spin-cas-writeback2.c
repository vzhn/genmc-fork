#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>

/*
 * As spin-cas-writeback0, with a pointer-typed expected value (a Treiber
 * stack push): only a retried push sees a head other than the first one.
 */

struct node {
	int val;
	struct node *next;
};

_Atomic(struct node *) top;
atomic_int retried;

static void push(int v)
{
	struct node *n = malloc(sizeof(*n));
	n->val = v;
	struct node *head = atomic_load_explicit(&top, memory_order_relaxed);
	struct node *first = head;
	do {
		n->next = head;
	} while (!atomic_compare_exchange_strong_explicit(&top, &head, n, memory_order_release,
							  memory_order_relaxed));
	if (head != first)
		atomic_fetch_add_explicit(&retried, 1, memory_order_relaxed);
}

static void *thread(void *arg)
{
	push((int)(intptr_t)arg);
	return NULL;
}

int main()
{
	pthread_t a, b;
	pthread_create(&a, NULL, thread, (void *)1);
	pthread_create(&b, NULL, thread, (void *)2);
	pthread_join(a, NULL);
	pthread_join(b, NULL);
	assert(atomic_load_explicit(&retried, memory_order_relaxed) != 1); /* fails iff a push was retried */
	return 0;
}
