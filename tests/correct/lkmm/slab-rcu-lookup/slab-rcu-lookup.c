#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <assert.h>
#include <lkmm.h>

/*
 * A lockless lookup in objects from a SLAB_TYPESAFE_BY_RCU cache (Linux
 * Documentation/RCU/rculist_nulls.rst), written with the kernel's
 * primitives (<lkmm.h>); tests/correct/classic/slab-rcu-lookup is the same
 * model in C11 atomics. Such objects can be freed and reused for another
 * key at once, within the reader's RCU read-side critical section, so a
 * lookup that found an object with the right key takes a reference with
 * atomic_inc_not_zero() and then checks the key again: "we must check key
 * after getting the reference on object". The writer initializes a reused
 * object's key before setting its refcount, with smp_wmb() between them.
 *
 * One bucket holds object O with key 1. The reader looks up key 1; the
 * writer deletes O (unlinks it and drops its last reference) and reuses it
 * for key 2. The reader gets either nothing or an object holding key 1.
 *
 * BUG_NO_RECHECK: the lookup does not check the key after taking the
 * reference. The reader finds O with key 1, the writer frees and reuses O
 * for key 2, and the reader's atomic_inc_not_zero() succeeds on the new
 * object: it returns an object holding key 2.
 */

struct object {
	int key;
	atomic_t refcnt;
};

struct object o;
struct object *bucket;

static void put_ref(struct object *obj)
{
	(void) atomic_fetch_sub_release(1, &obj->refcnt);	/* refcount_dec() */
}

static struct object *lookup(int key)
{
	struct object *obj;

	rcu_read_lock();
	for (;;) {
		obj = rcu_dereference(bucket);
		if (obj == NULL || READ_ONCE(obj->key) != key) {
			obj = NULL;
			break;
		}
		if (!atomic_inc_not_zero(&obj->refcnt)) {
			/* being freed: retry once the bucket or the object changes */
			__VERIFIER_await(READ_ONCE(bucket) != obj || atomic_read(&obj->refcnt) != 0);
			continue;
		}
#ifndef BUG_NO_RECHECK
		if (READ_ONCE(obj->key) != key) {
			/* freed and reused for another key: not ours */
			put_ref(obj);
			continue;
		}
#endif
		break;
	}
	rcu_read_unlock();
	return obj;
}

static void *reader(void *unused)
{
	struct object *obj = lookup(1);

	if (obj) {
		assert(READ_ONCE(obj->key) == 1);
		put_ref(obj);
	}
	return NULL;
}

static void *writer(void *unused)
{
	/* delete: unlink, drop the table's reference; the object is freed */
	WRITE_ONCE(bucket, NULL);
	if (atomic_dec_and_test(&o.refcnt)) {
		/* reuse the memory for key 2: the key first, then the refcount */
		WRITE_ONCE(o.key, 2);
		smp_wmb();
		atomic_set(&o.refcnt, 1);
		rcu_assign_pointer(bucket, &o);
	}
	return NULL;
}

int main()
{
	pthread_t r, w;

	o.key = 1;
	atomic_set(&o.refcnt, 1);
	bucket = &o;

	if (pthread_create(&r, NULL, reader, NULL))
		abort();
	if (pthread_create(&w, NULL, writer, NULL))
		abort();
	if (pthread_join(r, NULL))
		abort();
	if (pthread_join(w, NULL))
		abort();
	return 0;
}
