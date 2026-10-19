#include "../../../lib/go.h"

/*
 * GoBench kernel istio#8214 (non-blocking, data race), after Istio's
 * pkg/cache lruCache. Setting an entry counts the write with an atomic add
 * on the cache's stats; the mixer's check then reads the stats with a plain
 * copy. Two checks run concurrently.
 *
 * BUG_PLAIN_STATS_READ: Stats() copies the counters without
 * synchronisation, as before Istio PR #8214 ("Fix data race with
 * read/write stats in lruCache", 2018), which reads them under the cache's
 * read lock and updates them under its lock.
 */

struct lru_cache {
	go_rwmutex_t mu;
	unsigned long writes;
};

struct lru_cache c;
go_waitgroup_t wg;

static void set_with_expiration(void)
{
#ifdef BUG_PLAIN_STATS_READ
	__atomic_fetch_add(&c.writes, 1, __ATOMIC_SEQ_CST);
#else
	go_rwmutex_lock(&c.mu);
	c.writes++;
	go_rwmutex_unlock(&c.mu);
#endif
}

static unsigned long stats(void)
{
#ifdef BUG_PLAIN_STATS_READ
	return c.writes;
#else
	go_rwmutex_rlock(&c.mu);
	unsigned long w = c.writes;
	go_rwmutex_runlock(&c.mu);
	return w;
#endif
}

/* s.Check(): cache.Set() records the stats */
void *check(void *arg)
{
	set_with_expiration();
	(void) stats();
	go_wg_done(&wg);
	return NULL;
}

void *server(void *arg)
{
	go(check, NULL);
	go(check, NULL);
	go_wg_done(&wg);
	return NULL;
}

int main()
{
	go_init();
	go_wg_add(&wg, 3);
	go(server, NULL);
	go_wg_wait(&wg);
	return 0;
}
