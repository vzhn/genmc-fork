#include "../../../lib/go.h"

/*
 * GoBench kernel cockroach#6181 (blocking, recursive read lock), after
 * CockroachDB's kv.rangeDescriptorCache. A lookup takes the cache's RWMutex
 * for reading and logs the cache with fmt.Printf("%s", rdc), which calls the
 * cache's String() method, which takes the read lock again. Then the lookup
 * takes the lock for writing to fill the cache. NLOOKUPS lookups run
 * concurrently and the test waits for them with a WaitGroup.
 *
 * If another lookup's Lock() comes between one lookup's two RLock()s, the
 * reader waits for the writer, which waits for the reader.
 *
 * BUG_RECURSIVE_RLOCK: the log line calls String(), which locks, while the
 * read lock is held; the fixed version logs with stringLocked(), which
 * expects the lock held.
 */

#ifndef NLOOKUPS
# define NLOOKUPS 2
#endif

go_rwmutex_t range_cache_mu;
go_waitgroup_t wg;

static void string_locked(void)
{
}

static void string(void)
{
	go_rwmutex_rlock(&range_cache_mu);
	string_locked();
	go_rwmutex_runlock(&range_cache_mu);
}

static void lookup_range_descriptor(void)
{
	go_rwmutex_rlock(&range_cache_mu);
#ifdef BUG_RECURSIVE_RLOCK
	string();		/* fmt.Printf("...%s", rdc) */
#else
	string_locked();
#endif
	go_rwmutex_runlock(&range_cache_mu);
	go_rwmutex_lock(&range_cache_mu);
	go_rwmutex_unlock(&range_cache_mu);
}

void *lookup(void *arg)
{
	lookup_range_descriptor();
	go_wg_done(&wg);
	return NULL;
}

int main()
{
	go_init();
	for (int i = 0; i < NLOOKUPS; i++) {
		go_wg_add(&wg, 1);
		go(lookup, NULL);
	}
	go_wg_wait(&wg);
	(void) string;
	return 0;
}
