#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>

/*
 * A spin loop compares a loaded pointer with a global's address (#267): the
 * pass that annotates the loop's load aborted on the address. Nothing writes
 * &head, so the loop never ends.
 */

struct node {
	_Atomic(struct node *) next;
};
struct node head, n1;

void *walker(void *arg)
{
	while (atomic_load_explicit(&n1.next, memory_order_relaxed) != &head)
		;
	return NULL;
}

int main(void)
{
	pthread_t t;

	atomic_init(&n1.next, &n1);
	atomic_init(&head.next, &n1);
	pthread_create(&t, NULL, walker, NULL);
	pthread_join(t, NULL);
	return 0;
}
