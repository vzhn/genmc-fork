#include <stdlib.h>
#include <stddef.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>
#include "../../../lib/lock.h"

/*
 * Double-checked locking (Java Concurrency in Practice, Listing 16.7; The Art
 * of Multiprocessor Programming, Fig. 3.10): get() returns a shared object,
 * created on first use. A thread that finds the object published skips the
 * lock; otherwise it takes the lock, checks again and creates the object.
 * N threads call get(); each must see the object initialized, and it is
 * created once.
 *
 * Without a switch the object is published with a release store and read
 * with an acquire load.
 *
 * One switch per bug:
 * - BUG_RELAXED: relaxed publication. Correct under SC, but a thread that
 *   skips the lock can see the pointer before the object's contents: fails
 *   under RC11 and IMM;
 * - BUG_PLAIN: the pointer is a plain variable, read outside the lock: a
 *   data race;
 * - BUG_UNLOCKED: no lock and no second check (check-then-act, Listing 2.3):
 *   two threads can both create the object.
 */

#ifndef N
#define N 2
#endif

#ifdef BUG_RELAXED
#define LD memory_order_relaxed
#define ST memory_order_relaxed
#else
#define LD memory_order_acquire
#define ST memory_order_release
#endif

struct object {
	atomic_int value;
};

struct object storage;
atomic_int created;
lock_t lock;

#ifdef BUG_PLAIN
struct object *instance;
#define load_instance(mo) instance
#define store_instance(p, mo) (instance = (p))
#else
_Atomic(struct object *) instance;
#define load_instance(mo) atomic_load_explicit(&instance, mo)
#define store_instance(p, mo) atomic_store_explicit(&instance, p, mo)
#endif

static struct object *create(void)
{
	struct object *o = &storage;

	atomic_fetch_add_explicit(&created, 1, memory_order_relaxed);
	atomic_store_explicit(&o->value, 42, memory_order_relaxed);
	return o;
}

static struct object *get(void)
{
	struct object *o = load_instance(LD);

	if (o != NULL)
		return o;
#ifdef BUG_UNLOCKED
	o = create();
	store_instance(o, ST);
#else
	lock_acquire(&lock);
	o = load_instance(memory_order_relaxed);
	if (o == NULL) {
		o = create();
		store_instance(o, ST);
	}
	lock_release(&lock);
#endif
	return o;
}

static void *thread(void *unused)
{
	struct object *o = get();

	assert(atomic_load_explicit(&o->value, memory_order_relaxed) == 42);
	return NULL;
}

int main()
{
	pthread_t t[N];

	for (int i = 0; i < N; i++)
		if (pthread_create(&t[i], NULL, thread, NULL))
			abort();
	for (int i = 0; i < N; i++)
		if (pthread_join(t[i], NULL))
			abort();
	assert(atomic_load_explicit(&created, memory_order_relaxed) == 1);
	return 0;
}
