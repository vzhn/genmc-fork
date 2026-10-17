#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>

/*
 * A lockless lookup in objects from a SLAB_TYPESAFE_BY_RCU cache (Linux
 * Documentation/RCU/rculist_nulls.rst). Such objects can be freed and
 * reused for another key at once, within the reader's RCU read-side
 * section, so a lookup that found an object with the right key takes a
 * reference with atomic_inc_not_zero() and then checks the key again: "we
 * must check key after getting the reference on object". The writer
 * initializes a reused object's key before setting its refcount with
 * release, and the lookup's reference is acquire.
 *
 * One bucket holds object O with key 1. The reader looks up key 1; the
 * writer deletes O (unlinks it and drops its last reference) and reuses it
 * for key 2. The reader gets either nothing or an object holding key 1.
 *
 * BUG_NO_RECHECK: the lookup does not check the key after taking the
 * reference. The reader finds O with key 1, the writer frees and reuses O
 * for key 2, and the reader's atomic_inc_not_zero() succeeds on the new
 * object: it returns an object holding key 2. Fails under every model.
 */

struct object {
	atomic_int key;
	atomic_int refcnt;
};

struct object o;
_Atomic(struct object *) bucket;

static bool try_get_ref(struct object *obj)
{
	int c = atomic_load_explicit(&obj->refcnt, memory_order_relaxed);

	do {
		if (c == 0)
			return false;	/* being freed */
	} while (!atomic_compare_exchange_strong_explicit(&obj->refcnt, &c, c + 1,
							   memory_order_acquire,
							   memory_order_relaxed));
	return true;
}

static void put_ref(struct object *obj)
{
	atomic_fetch_sub_explicit(&obj->refcnt, 1, memory_order_release);
}

static struct object *lookup(int key)
{
	/* rcu_read_lock() */
	for (;;) {
		struct object *obj = atomic_load_explicit(&bucket, memory_order_acquire);

		if (obj == NULL ||
		    atomic_load_explicit(&obj->key, memory_order_relaxed) != key)
			return NULL;
		if (!try_get_ref(obj)) {
			/* being freed: retry once the bucket or the object changes */
			__VERIFIER_await(atomic_load_explicit(&bucket, memory_order_relaxed) != obj ||
					 atomic_load_explicit(&obj->refcnt, memory_order_relaxed) != 0);
			continue;
		}
#ifndef BUG_NO_RECHECK
		if (atomic_load_explicit(&obj->key, memory_order_relaxed) != key) {
			/* freed and reused for another key: not ours */
			put_ref(obj);
			continue;
		}
#endif
		return obj;
	}
	/* rcu_read_unlock() */
}

static void *reader(void *unused)
{
	struct object *obj = lookup(1);

	if (obj) {
		assert(atomic_load_explicit(&obj->key, memory_order_relaxed) == 1);
		put_ref(obj);
	}
	return NULL;
}

static void *writer(void *unused)
{
	/* delete: unlink, drop the table's reference; the object is freed */
	atomic_store_explicit(&bucket, NULL, memory_order_release);
	if (atomic_fetch_sub_explicit(&o.refcnt, 1, memory_order_release) == 1) {
		/* reuse the memory for key 2: the key first, then the refcount */
		atomic_store_explicit(&o.key, 2, memory_order_relaxed);
		atomic_store_explicit(&o.refcnt, 1, memory_order_release);
		atomic_store_explicit(&bucket, &o, memory_order_release);
	}
	return NULL;
}

int main()
{
	pthread_t r, w;

	atomic_init(&o.key, 1);
	atomic_init(&o.refcnt, 1);
	atomic_init(&bucket, &o);

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
