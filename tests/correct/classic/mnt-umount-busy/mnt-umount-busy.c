#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>

/*
 * A synchronous umount racing with an RCU path walk that legitimizes its
 * reference to the mount (fs/namespace.c do_umount(), __legitimize_mnt()).
 * The walker started under mount_lock's sequence count. To keep the mount
 * it adds to the mount's refcount, issues smp_mb() and checks that the
 * sequence count has not moved; if it has, it gives the reference back.
 * do_umount() takes lock_mount_hash(), which bumps the sequence count,
 * and then checks the refcount: if nobody else holds the mount, it is
 * unmounted. Kernel primitives map to C11: mnt_add_count() and the
 * sequence-count accesses are relaxed, smp_mb() is a seq_cst fence.
 *
 * Either the walker sees the bumped sequence count or do_umount() sees the
 * walker's reference: never both "legitimized" and "unmounted".
 *
 * BUG_NO_UMOUNT_MB: do_umount() reads the refcount right after taking the
 * lock, without smp_mb(), as before commit 65781e19dcfc ("do_umount(): add
 * missing barrier before refcount checks in sync case", 2025). With the
 * walker this is store buffering: each side can miss the other's store,
 * and the walker keeps a reference to a mount that is gone. Passes under
 * SC.
 */

atomic_int mount_seq;		/* even: the value the walk started under */
atomic_int mnt_count;		/* references besides the umounter's */
atomic_bool legitimized, unmounted;

static void *walker(void *arg)
{
	/* __legitimize_mnt() */
	atomic_fetch_add_explicit(&mnt_count, 1, memory_order_relaxed);
	atomic_thread_fence(memory_order_seq_cst);
	if (atomic_load_explicit(&mount_seq, memory_order_relaxed) == 0)
		atomic_store_explicit(&legitimized, true, memory_order_relaxed);
	else
		atomic_fetch_sub_explicit(&mnt_count, 1, memory_order_relaxed);
	return NULL;
}

static void *umounter(void *arg)
{
	/* lock_mount_hash() */
	atomic_store_explicit(&mount_seq, 1, memory_order_relaxed);
#ifndef BUG_NO_UMOUNT_MB
	atomic_thread_fence(memory_order_seq_cst);
#endif
	/* propagate_mount_busy() */
	if (atomic_load_explicit(&mnt_count, memory_order_relaxed) == 0)
		atomic_store_explicit(&unmounted, true, memory_order_relaxed);
	return NULL;
}

int main()
{
	pthread_t w, u;

	if (pthread_create(&w, NULL, walker, NULL) ||
	    pthread_create(&u, NULL, umounter, NULL))
		abort();
	if (pthread_join(w, NULL) || pthread_join(u, NULL))
		abort();
	assert(!(atomic_load_explicit(&legitimized, memory_order_relaxed) &&
		 atomic_load_explicit(&unmounted, memory_order_relaxed)));
	return 0;
}
