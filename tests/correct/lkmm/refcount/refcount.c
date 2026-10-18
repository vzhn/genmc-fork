#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <assert.h>
#include <lkmm.h>

/*
 * Dropping the last reference to an object, as the Linux kernel's
 * refcount_dec_and_test() does (lib/refcount.c), written with the kernel's
 * primitives (<lkmm.h>); tests/correct/classic/refcount is the same model
 * in C11 atomics. Every decrement is a release, so a thread's uses of the
 * object come before its decrement, and the thread that brings the count
 * to zero acquires (smp_acquire__after_ctrl_dep()) before it destroys the
 * object, so it sees every other thread's uses.
 *
 * The object has two references, one per thread. Each thread writes its own
 * field and drops its reference; whoever drops the last one runs the
 * destructor, which reads both fields, and frees the object.
 *
 * BUG_NO_ACQUIRE: refcount_dec_and_test() before Linux commit 47b8f3ab9c49
 * ("refcount_t: Add ACQUIRE ordering on success for dec(sub)_and_test()
 * variants", 2019) was a release decrement only. The destructor's read of
 * the other thread's field is then not ordered after its write: a data race
 * (and the free is not ordered after it either).
 */

struct object {
	int field[2];
	atomic_t refs;
};

struct object *obj;
int sum;

static bool refcount_dec_and_test(atomic_t *refs)
{
	if (atomic_fetch_sub_release(1, refs) != 1)
		return false;
#ifndef BUG_NO_ACQUIRE
	smp_acquire__after_ctrl_dep();
#endif
	return true;
}

static void put(struct object *o)
{
	if (refcount_dec_and_test(&o->refs)) {
		sum = o->field[0] + o->field[1];	/* the destructor */
		free(o);
	}
}

static void *thread(void *arg)
{
	int me = (long) arg;

	obj->field[me] = me + 1;
	put(obj);
	return NULL;
}

int main()
{
	pthread_t a, b;

	obj = malloc(sizeof(*obj));
	obj->field[0] = obj->field[1] = 0;
	atomic_set(&obj->refs, 2);

	if (pthread_create(&a, NULL, thread, (void *) 0L))
		abort();
	if (pthread_create(&b, NULL, thread, (void *) 1L))
		abort();
	if (pthread_join(a, NULL))
		abort();
	if (pthread_join(b, NULL))
		abort();
	assert(sum == 3);
	return 0;
}
