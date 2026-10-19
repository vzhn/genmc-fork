#include "../../../lib/go.h"

/*
 * GoBench kernel istio#8144 (non-blocking, data race), after Istio's
 * pkg/cache TTL cache. The cache's evicter goroutine calls the eviction
 * callback for its entries; the test's callback counts the calls, and the
 * test reads the count.
 *
 * BUG_PLAIN_COUNTER: the callback increments a plain counter, as before
 * Istio commit 39f7d4745178 ("Fix data races in pkg/cache", 2018), which
 * makes it atomic.
 */

#ifdef BUG_PLAIN_COUNTER
int callbacks;
# define INC_CALLBACKS() (callbacks++)
# define LOAD_CALLBACKS() (callbacks)
#else
atomic_long callbacks;
# define INC_CALLBACKS() atomic_fetch_add(&callbacks, 1)
# define LOAD_CALLBACKS() atomic_load(&callbacks)
#endif

go_waitgroup_t wg;
bool entry_stored;	/* sync.Map: a single entry */
go_mutex_t entries_mu;

/* c.evicter() -> evictExpired() */
void *evicter(void *arg)
{
	go_mutex_lock(&entries_mu);
	bool any = entry_stored;
	go_mutex_unlock(&entries_mu);
	if (any)
		INC_CALLBACKS();
	return NULL;
}

void *test(void *arg)
{
	go(evicter, NULL);		/* NewTTLWithCallback() */
	go_mutex_lock(&entries_mu);	/* SetWithExpiration() */
	entry_stored = true;
	go_mutex_unlock(&entries_mu);
	(void) LOAD_CALLBACKS();
	go_wg_done(&wg);
	return NULL;
}

int main()
{
	go_init();
	go_wg_add(&wg, 1);
	go(test, NULL);
	go_wg_wait(&wg);
	return 0;
}
