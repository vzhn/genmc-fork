#include "../../../lib/go.h"

/*
 * GoBench kernel kubernetes#13135 (blocking, AB-BA deadlock), after the
 * apiserver's storage Cacher and its WatchCache. Each has a lock, and each
 * calls into the other through hooks:
 *
 * - startCaching(), run in a loop until the stop channel closes, takes the
 *   Cacher's lock and relists, which replaces the WatchCache's contents
 *   under the WatchCache's lock; the onReplace hook marks the Cacher
 *   initialized and releases the Cacher's lock.
 * - Adding an event takes the WatchCache's lock and calls the onEvent hook,
 *   the Cacher's processEvent(), which takes the Cacher's lock.
 *
 * Main creates the Cacher and waits until it is initialized, then adds an
 * event while closing the stop channel. The model runs startCaching() up to
 * NRELIST times and then only waits for stop.
 *
 * BUG_CACHER_LOCK_IN_START: startCaching() takes the Cacher's lock before
 * the WatchCache's, the reverse of adding an event, as before Kubernetes
 * commit a12b7edc42c5 ("Fix deadlock in Cacher on etcd error", 2015),
 * which guards startCaching() with a separate "usable" lock that event
 * processing does not take.
 */

#ifndef NRELIST
# define NRELIST 2
#endif

go_mutex_t cacher_mu;
go_mutex_t cacher_usable;	/* the fix */
go_rwmutex_t watch_cache_mu;
go_waitgroup_t initialized;
go_once_t init_once;
go_chan_t *stop_channel;

static void initialized_done(void *arg)
{
	go_wg_done(&initialized);
}

/* the onReplace hook */
static void on_replace(void)
{
	go_once_do(&init_once, initialized_done, NULL);
#ifdef BUG_CACHER_LOCK_IN_START
	go_mutex_unlock(&cacher_mu);
#else
	go_mutex_unlock(&cacher_usable);
#endif
}

/* the onEvent hook: c.processEvent() */
static void on_event(void)
{
	go_mutex_lock(&cacher_mu);
	go_mutex_unlock(&cacher_mu);
}

/* WatchCache.Replace() */
static void replace(void)
{
	go_rwmutex_lock(&watch_cache_mu);
	on_replace();
	go_rwmutex_unlock(&watch_cache_mu);
}

static void start_caching(void)
{
#ifdef BUG_CACHER_LOCK_IN_START
	go_mutex_lock(&cacher_mu);
#else
	go_mutex_lock(&cacher_usable);
#endif
	replace();		/* reflector.ListAndWatch() -> syncWith() */
}

/* G2: wait.Until(startCaching, 0, stopCh) */
void *until(void *arg)
{
	for (int i = 0; i < NRELIST; i++) {
		struct go_case cs[] = { GO_RECV(stop_channel, NULL) };

		if (go_select(cs, 1, true) == 0)
			return NULL;
		start_caching();
	}
	go_recv(stop_channel);
	return NULL;
}

/* G3: c.watchCache.Add() */
void *add(void *arg)
{
	go_rwmutex_lock(&watch_cache_mu);
	on_event();
	go_rwmutex_unlock(&watch_cache_mu);
	return NULL;
}

void *close_stop(void *arg)
{
	go_close(stop_channel);
	return NULL;
}

int main()
{
	go_init();
	stop_channel = go_make_chan(0);

	/* NewCacher() */
	go_wg_add(&initialized, 1);
	go(until, NULL);
	go_wg_wait(&initialized);

	go(add, NULL);
	go(close_stop, NULL);
	return 0;
}
