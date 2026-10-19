#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>

/*
 * Turning a negative dentry positive while a lockless path walk checks it
 * (fs/dcache.c __d_set_inode_and_type(), fs/namei.c). The instantiating
 * thread stores ->d_inode, then the type bits in ->d_flags. A walker that
 * holds a reference but no lock reads ->d_flags; if the type says
 * positive, it goes on to use ->d_inode. Kernel primitives map to C11:
 * READ_ONCE()/WRITE_ONCE() are relaxed accesses, smp_store_release() and
 * smp_load_acquire() release and acquire accesses.
 *
 * A walker that finds the dentry positive must find its inode.
 *
 * BUG_ONCE_FLAGS: ->d_flags is written with WRITE_ONCE() and read with
 * READ_ONCE(), as before commit 2fa6b1e01a9b ("fs/namei.c: fix missing
 * barriers when checking positivity", 2019). The walker can see the
 * positive type and still read a NULL ->d_inode. Passes under SC.
 */

#define DCACHE_ENTRY_TYPE	0x7
#define DCACHE_REGULAR_TYPE	0x4

#ifdef BUG_ONCE_FLAGS
# define FLAGS_STORE memory_order_relaxed
# define FLAGS_LOAD memory_order_relaxed
#else
# define FLAGS_STORE memory_order_release
# define FLAGS_LOAD memory_order_acquire
#endif

struct inode {
	int i_mode;
};

struct dentry {
	atomic_uint d_flags;
	_Atomic(struct inode *) d_inode;
};

struct inode the_inode;
struct dentry dentry;

static void d_instantiate(struct inode *inode)
{
	/* __d_set_inode_and_type() */
	atomic_store_explicit(&dentry.d_inode, inode, memory_order_relaxed);
	unsigned flags = atomic_load_explicit(&dentry.d_flags, memory_order_relaxed);
	flags = (flags & ~DCACHE_ENTRY_TYPE) | DCACHE_REGULAR_TYPE;
	atomic_store_explicit(&dentry.d_flags, flags, FLAGS_STORE);
}

static void *creator(void *arg)
{
	d_instantiate(&the_inode);
	return NULL;
}

static void *walker(void *arg)
{
	unsigned flags = atomic_load_explicit(&dentry.d_flags, FLAGS_LOAD);

	if ((flags & DCACHE_ENTRY_TYPE) == 0)
		return NULL;	/* negative: -ENOENT */
	assert(atomic_load_explicit(&dentry.d_inode, memory_order_relaxed) != NULL);
	return NULL;
}

int main()
{
	pthread_t c, w;

	if (pthread_create(&c, NULL, creator, NULL) ||
	    pthread_create(&w, NULL, walker, NULL))
		abort();
	if (pthread_join(c, NULL) || pthread_join(w, NULL))
		abort();
	return 0;
}
