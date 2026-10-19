#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>
#include "../../../lib/lock.h"

/*
 * The lockless lookup of inet_getpeer() (net/ipv4/inetpeer.c), reduced to
 * a tree of one node. Writers insert under base->lock, a seqlock: the
 * writer bumps the sequence count to odd, initializes the new peer
 * (refcount 1), links it into the rbtree with a plain store and bumps the
 * count back to even. A reader first looks the peer up without the lock
 * between read_seqbegin() and read_seqretry(); if that fails, it looks up
 * again under the lock. Kernel primitives map to C11: the sequence count
 * and the tree accesses are relaxed, smp_wmb() a release fence, smp_rmb()
 * an acquire fence, and the seqlock's spinlock a lock_t. The kernel reads
 * the tree with rcu_dereference_raw(), an address dependency; the reader's
 * acquire fences make up for it here.
 *
 * Peers come from a slab cache, so a new one can still hold the refcount
 * of its previous life. A peer that a reader uses must have refcount 1.
 *
 * BUG_NO_SEQRETRY: a peer found by the lockless lookup is returned without
 * read_seqretry(), as before commit 67ef49047d31 ("inetpeer: add a missing
 * read_seqretry() in inet_getpeer()", 2026). Nothing orders the peer's
 * initialization before its link, so the reader can find the peer and
 * read its stale refcount (a use after free in the kernel, once
 * refcount_inc_not_zero() succeeds on it). Passes under SC.
 */

#define STALE_REFCNT 7

struct inet_peer {
	atomic_int refcnt;
};

struct inet_peer slab_peer;
_Atomic(struct inet_peer *) rb_root;
atomic_uint seq;
lock_t base_lock;

static void *inserter(void *arg)
{
	/* write_seqlock_bh() */
	lock_acquire(&base_lock);
	atomic_store_explicit(&seq, atomic_load_explicit(&seq, memory_order_relaxed) + 1,
			      memory_order_relaxed);
	atomic_thread_fence(memory_order_release);
	/* kmem_cache_alloc(), refcount_set(), rb_link_node() */
	atomic_store_explicit(&slab_peer.refcnt, 1, memory_order_relaxed);
	atomic_store_explicit(&rb_root, &slab_peer, memory_order_relaxed);
	/* write_sequnlock_bh() */
	atomic_thread_fence(memory_order_release);
	atomic_store_explicit(&seq, atomic_load_explicit(&seq, memory_order_relaxed) + 1,
			      memory_order_relaxed);
	lock_release(&base_lock);
	return NULL;
}

static struct inet_peer *inet_getpeer(void)
{
	/* read_seqbegin() */
	unsigned s = atomic_load_explicit(&seq, memory_order_relaxed);
	atomic_thread_fence(memory_order_acquire);

	if (!(s & 1)) {
		struct inet_peer *p = atomic_load_explicit(&rb_root, memory_order_relaxed);
#ifdef BUG_NO_SEQRETRY
		if (p)
			return p;
#else
		/* read_seqretry() */
		atomic_thread_fence(memory_order_acquire);
		if (p && atomic_load_explicit(&seq, memory_order_relaxed) == s)
			return p;
#endif
	}
	/* the locked lookup; a miss would insert, left out here */
	lock_acquire(&base_lock);
	struct inet_peer *p = atomic_load_explicit(&rb_root, memory_order_relaxed);
	lock_release(&base_lock);
	return p;
}

static void *reader(void *arg)
{
	struct inet_peer *p = inet_getpeer();

	if (p)
		assert(atomic_load_explicit(&p->refcnt, memory_order_relaxed) == 1);
	return NULL;
}

int main()
{
	pthread_t w, r;

	atomic_init(&slab_peer.refcnt, STALE_REFCNT);
	if (pthread_create(&w, NULL, inserter, NULL) ||
	    pthread_create(&r, NULL, reader, NULL))
		abort();
	if (pthread_join(w, NULL) || pthread_join(r, NULL))
		abort();
	return 0;
}
