#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>
#include "../../../lib/lock.h"

/*
 * Publishing a new anon_vma while a fault on a neighbouring VMA tries to
 * reuse it (mm/rmap.c __anon_vma_prepare(), mm/vma.c reusable_anon_vma()).
 * Thread A allocates an anon_vma, points its ->root at itself and stores
 * it into vma->anon_vma. Thread B, faulting on an adjacent compatible VMA,
 * reads that pointer with READ_ONCE() and locks the anon_vma, which locks
 * ->root's rwsem; unlocking reads ->root again. Kernel primitives map to
 * C11: READ_ONCE()/WRITE_ONCE() and plain pointer stores are relaxed,
 * smp_store_release() a release store, and the rwsem a lock_t.
 *
 * The kernel orders B's reads of ->root after its read of the pointer by
 * the address dependency. C11 has no dependency ordering, so B's read of
 * the pointer is acquire here.
 *
 * Anon_vmas come from a SLAB_TYPESAFE_BY_RCU cache, so a new one can
 * still hold the ->root of its previous life. B must lock and unlock the
 * same root.
 *
 * BUG_PLAIN_PUBLISH: vma->anon_vma = anon_vma is a plain store, as before
 * commit b6ac0b3f6013 ("mm/rmap: fix missing barrier between anon_vma init
 * and vma->anon_vma publish", 2026). B can find the new anon_vma, lock the
 * stale root and unlock the new one: one rwsem stays locked forever and
 * the other is released by a task that never took it (the hung task in
 * the commit). Passes under SC.
 */

struct anon_vma {
	_Atomic(struct anon_vma *) root;
	lock_t rwsem;
};

#ifdef BUG_PLAIN_PUBLISH
# define PUBLISH_ORDER memory_order_relaxed
#else
# define PUBLISH_ORDER memory_order_release
#endif

/* slab[0]: the recycled anon_vma, whose stale root is slab[1] */
struct anon_vma slab[2];
_Atomic(struct anon_vma *) vma_anon_vma;

static void *prepare(void *arg)
{
	struct anon_vma *av = &slab[0];

	/* anon_vma_alloc() */
	atomic_store_explicit(&av->root, av, memory_order_relaxed);
	atomic_store_explicit(&vma_anon_vma, av, PUBLISH_ORDER);
	return NULL;
}

static void *reuse(void *arg)
{
	/* READ_ONCE(); acquire stands in for the address dependency */
	struct anon_vma *av = atomic_load_explicit(&vma_anon_vma, memory_order_acquire);

	if (av == NULL)
		return NULL;

	/* anon_vma_lock_write() */
	struct anon_vma *locked = atomic_load_explicit(&av->root, memory_order_relaxed);
	lock_acquire(&locked->rwsem);
	/* anon_vma_unlock_write() */
	struct anon_vma *unlocked = atomic_load_explicit(&av->root, memory_order_relaxed);
	assert(unlocked == locked);
	lock_release(&unlocked->rwsem);
	return NULL;
}

int main()
{
	pthread_t a, b;

	atomic_init(&slab[0].root, &slab[1]);
	if (pthread_create(&a, NULL, prepare, NULL) ||
	    pthread_create(&b, NULL, reuse, NULL))
		abort();
	if (pthread_join(a, NULL) || pthread_join(b, NULL))
		abort();
	return 0;
}
