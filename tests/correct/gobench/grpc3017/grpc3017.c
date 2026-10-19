#include "../../../lib/go.h"

/*
 * GoBench kernel grpc#3017 (blocking, missing unlock), after grpc-go's
 * lbCacheClientConn (balancer/grpclb). Removing a SubConn does not close it
 * at once: it goes into a cache and a timer (time.AfterFunc) deletes it
 * later, under the cache's mutex. Creating a SubConn for a cached address
 * takes it back and cancels the timer; if the timer has already fired, the
 * cancel can only mark the entry abortDeleting, and the timer's callback,
 * once it gets the mutex, leaves the entry alone.
 *
 * That early return in the callback forgot to unlock the mutex, so the
 * next RemoveSubConn() or NewSubConn() blocks forever. A goroutine removes
 * and recreates the SubConn NITER times while the test waits for it.
 *
 * time.AfterFunc() is a goroutine here; Stop() succeeds only while the
 * callback has not started.
 *
 * BUG_ABORT_WITHOUT_UNLOCK: the callback returns without unlocking when the
 * entry is marked abortDeleting, as before grpc-go PR #3017 (2019), which
 * defers the unlock.
 */

#ifndef NITER
# define NITER 2
#endif

struct entry {
	int sc;
	atomic_int timer;	/* 0 pending, 1 fired, 2 stopped */
	bool abort_deleting;
};

go_mutex_t mu;
struct entry entries[NITER];
int nentries;
struct entry *sub_conn_cache;	/* the cache entry for our single address */
bool sub_conn_to_addr;		/* subConnToAddr has our SubConn */

void *timer_func(void *arg)
{
	struct entry *e = arg;
	int pending = 0;

	if (!atomic_compare_exchange_strong(&e->timer, &pending, 1))
		return NULL;	/* stopped */
	go_mutex_lock(&mu);
	if (e->abort_deleting) {
#ifndef BUG_ABORT_WITHOUT_UNLOCK
		go_mutex_unlock(&mu);
#endif
		return NULL;
	}
	sub_conn_to_addr = false;
	sub_conn_cache = NULL;
	go_mutex_unlock(&mu);
	return NULL;
}

/* entry.cancel() */
static void cancel(struct entry *e)
{
	int pending = 0;

	if (!atomic_compare_exchange_strong(&e->timer, &pending, 2))	/* !timer.Stop() */
		e->abort_deleting = true;
}

static int new_sub_conn(void)
{
	go_mutex_lock(&mu);
	struct entry *e = sub_conn_cache;
	if (e) {
		cancel(e);
		sub_conn_cache = NULL;
		go_mutex_unlock(&mu);
		return e->sc;
	}
	sub_conn_to_addr = true;
	go_mutex_unlock(&mu);
	return 1;
}

static void remove_sub_conn(int sc)
{
	go_mutex_lock(&mu);
	if (!sub_conn_to_addr || sub_conn_cache) {
		go_mutex_unlock(&mu);
		return;
	}
	struct entry *e = &entries[nentries++];
	e->sc = sc;
	sub_conn_cache = e;
	go(timer_func, e);	/* time.AfterFunc(ccc.timeout, ...) */
	go_mutex_unlock(&mu);
}

go_chan_t *done;

void *churn_and_close(void *arg)
{
	int sc = (int)(long) arg;

	for (int i = 0; i < NITER; i++) {
		remove_sub_conn(sc);
		sc = new_sub_conn();
	}
	go_close(done);
	return NULL;
}

int main()
{
	go_init();
	done = go_make_chan(0);
	int sc = new_sub_conn();
	go(churn_and_close, (void *)(long) sc);
	go_recv(done);
	return 0;
}
