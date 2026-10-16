#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>

/*
 * Function pointers that live in memory, explored with several threads
 * (args.*.in): each worker interprets its own copy of the module, but the
 * workers exchange graphs and so the pointer values stored in memory.
 *
 * - main stores {fn, arg} in a struct and a trampoline calls fn(arg), as a
 *   thread pool does;
 * - a statically initialized table holds a second function pointer;
 * - the stored pointers are compared against the functions themselves.
 *
 * The called functions make calls GenMC models internally (a mutex), which
 * is where a pointer into another worker's module used to fail.
 */

struct job {
	void (*fn)(void *);
	void *arg;
};

static pthread_mutex_t lock;
static int counter;
static atomic_int x, y;

static void work(void *arg)
{
	pthread_mutex_lock(&lock);
	counter++;
	pthread_mutex_unlock(&lock);
	atomic_store_explicit((atomic_int *)arg, 1, memory_order_relaxed);
}

static void finish(void *arg)
{
	pthread_mutex_lock(&lock);
	counter += 10;
	pthread_mutex_unlock(&lock);
}

static void (*const table[])(void *) = {work, finish};

static void *trampoline(void *arg)
{
	struct job *j = arg;

	assert(j->fn == work && j->fn == table[0]);
	j->fn(j->arg);
	table[1](NULL);
	return NULL;
}

int main()
{
	struct job jobs[2] = {{work, &x}, {table[0], &y}};
	pthread_t t[2];

	pthread_mutex_init(&lock, NULL);
	for (int i = 0; i < 2; i++)
		pthread_create(&t[i], NULL, trampoline, &jobs[i]);
	int a = atomic_load_explicit(&x, memory_order_relaxed);
	int b = atomic_load_explicit(&y, memory_order_relaxed);
	for (int i = 0; i < 2; i++)
		pthread_join(t[i], NULL);
	assert(counter == 22);
	(void)a;
	(void)b;
	return 0;
}
