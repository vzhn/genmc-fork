#include "../../../lib/go.h"

/*
 * GoBench kernel hugo#3251 (blocking, AB-BA deadlock), after Hugo's remote
 * resource cache (helpers/remote.go). A per-URL mutex serialises downloads
 * of one URL; the map of those mutexes is guarded by an RWMutex.
 * URLLock() takes the map's lock for writing and, holding it, locks the
 * URL's mutex; URLUnlock() takes the map's lock for reading to find and
 * unlock the URL's mutex.
 *
 * With two goroutines fetching the same URL, one can hold the URL's mutex
 * and wait for the read lock in URLUnlock() while the other holds the
 * write lock and waits for the URL's mutex. NGOR goroutines fetch NFETCH
 * times each.
 *
 * BUG_URL_LOCK_UNDER_MAP_LOCK: URLLock() locks the URL's mutex while
 * holding the map's lock, as before Hugo PR #3251 (2017), which releases
 * the map's lock first.
 */

#ifndef NGOR
# define NGOR 2
#endif
#ifndef NFETCH
# define NFETCH 1
#endif

go_rwmutex_t remote_url_lock;
go_mutex_t url_mutex;		/* l.m[url] */
bool url_mutex_exists;
go_waitgroup_t wg;

static void url_lock(void)
{
	go_rwmutex_lock(&remote_url_lock);
	url_mutex_exists = true;
#ifdef BUG_URL_LOCK_UNDER_MAP_LOCK
	go_mutex_lock(&url_mutex);
	go_rwmutex_unlock(&remote_url_lock);
#else
	go_rwmutex_unlock(&remote_url_lock);
	go_mutex_lock(&url_mutex);
#endif
}

static void url_unlock(void)
{
	go_rwmutex_rlock(&remote_url_lock);
	if (url_mutex_exists)
		go_mutex_unlock(&url_mutex);
	go_rwmutex_runlock(&remote_url_lock);
}

void *fetcher(void *arg)
{
	for (int j = 0; j < NFETCH; j++) {
		url_lock();		/* resGetRemote() */
		url_unlock();
	}
	go_wg_done(&wg);
	return NULL;
}

int main()
{
	go_init();
	go_wg_add(&wg, NGOR);
	for (int i = 0; i < NGOR; i++)
		go(fetcher, NULL);
	go_wg_wait(&wg);
	return 0;
}
