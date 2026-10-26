#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>

/*
 * With -svcomp, a read of uninitialized memory reads 0, as SV-COMP
 * defines it, instead of being reported. The block is malloc()ed but not
 * initialized: the reader reads b[0], which nobody writes, and b[1] either
 * before or after the writer's store; it also reads a local of main's that
 * main never initialized. Without the option, the first read is an error.
 */

static _Atomic(int) *b;
static int *local;

void *reader(void *arg)
{
	assert(atomic_load_explicit(&b[0], memory_order_relaxed) == 0);
	int v = atomic_load_explicit(&b[1], memory_order_relaxed);
	assert(v == 0 || v == 5);
	assert(*local == 0);
	return NULL;
}

void *writer(void *arg)
{
	atomic_store_explicit(&b[1], 5, memory_order_relaxed);
	return NULL;
}

int main()
{
	pthread_t r, w;
	int x;

	b = malloc(2 * sizeof(*b));
	local = &x;
	pthread_create(&r, NULL, reader, NULL);
	pthread_create(&w, NULL, writer, NULL);
	pthread_join(r, NULL);
	pthread_join(w, NULL);
	free(b);
	return 0;
}
