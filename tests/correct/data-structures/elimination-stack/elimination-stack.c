#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <assert.h>

#include "my_stack.c"

/*
 * A client of the elimination-backoff stack (my_stack.c). main pushes 1;
 * then NPUSH threads push 2, 3, ... while NPOP threads pop; main pops the
 * rest. Every value is popped exactly once. (Nodes leak, as in the
 * relinche client.)
 */

#ifndef NPUSH
# define NPUSH 1
#endif
#ifndef NPOP
# define NPOP 2
#endif

mystack_t s;
atomic_int seen[NPUSH + 2];

static void got(unsigned int v)
{
	assert(v >= 1 && v <= NPUSH + 1);
	int before = atomic_fetch_add_explicit(&seen[v], 1, memory_order_relaxed);
	assert(before == 0);
}

static void *pusher(void *arg)
{
	push(&s, (intptr_t) arg);
	return NULL;
}

static void *popper(void *unused)
{
	unsigned int v = pop(&s);

	if (v)
		got(v);
	return NULL;
}

int main()
{
	pthread_t p[NPUSH], c[NPOP];
	unsigned int v;

	init_stack(&s, 4);
	push(&s, 1);
	for (intptr_t i = 0; i < NPUSH; i++)
		if (pthread_create(&p[i], NULL, pusher, (void *) (i + 2)))
			abort();
	for (int i = 0; i < NPOP; i++)
		if (pthread_create(&c[i], NULL, popper, NULL))
			abort();
	for (int i = 0; i < NPUSH; i++)
		if (pthread_join(p[i], NULL))
			abort();
	for (int i = 0; i < NPOP; i++)
		if (pthread_join(c[i], NULL))
			abort();

	while ((v = pop(&s)))
		got(v);
	for (int i = 1; i <= NPUSH + 1; i++)
		assert(seen[i] == 1);
	return 0;
}
