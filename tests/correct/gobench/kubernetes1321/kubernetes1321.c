#include "../../../lib/go.h"

/*
 * GoBench kernel kubernetes#1321 (blocking, channel and lock), after
 * Kubernetes' pkg/watch.Mux. The mux's loop distributes events: holding
 * m.lock, it sends each event to every watcher's result channel. A watcher's
 * Stop() takes m.lock, removes the watcher and closes its result channel.
 *
 * If nobody reads the watcher's results, distribute() blocks sending while
 * holding m.lock, and Stop() cannot take the lock to close the channel.
 *
 * BUG_NO_STOPPED_CHAN: distribute() only sends, as before Kubernetes commit
 * 435e0b73bb99 ("pkg/watch: fix potential deadlock", 2014). The fix gives
 * each watcher a stopped channel, which Stop() closes before taking the
 * lock, and distribute() selects between sending and stopped.
 */

#define NEVENTS 2

struct mux_watcher {
	go_chan_t *result;
	go_chan_t *stopped;
	go_once_t stop;
};

go_mutex_t global_mtx;
go_mutex_t mux_lock;
struct mux_watcher *watcher;	/* m.watchers, at most one */

static void distribute(void)
{
	go_mutex_lock(&mux_lock);
	go_mutex_lock(&global_mtx);
	struct mux_watcher *w = watcher;
	if (w) {
#ifdef BUG_NO_STOPPED_CHAN
		go_send(w->result, 1);
#else
		struct go_case cs[] = { GO_SEND(w->result, 1), GO_RECV(w->stopped, NULL) };
		go_select(cs, 2, false);
#endif
	}
	go_mutex_unlock(&global_mtx);
	go_mutex_unlock(&mux_lock);
}

/* G2: m.loop() */
void *loop(void *arg)
{
	for (int i = 0; i < NEVENTS; i++)
		distribute();
	return NULL;
}

static void stop_watching(struct mux_watcher *w)
{
	go_mutex_lock(&mux_lock);
	if (watcher == w) {
		watcher = NULL;
		go_close(w->result);
	}
	go_mutex_unlock(&mux_lock);
}

static void stop_once(void *arg)
{
	struct mux_watcher *w = arg;

	go_close(w->stopped);
	stop_watching(w);
}

static void watcher_stop(struct mux_watcher *w)
{
#ifdef BUG_NO_STOPPED_CHAN
	stop_watching(w);
#else
	go_once_do(&w->stop, stop_once, w);
#endif
}

struct mux_watcher the_watcher;

/* G1: testMuxWatcherClose() */
void *test_mux_watcher_close(void *arg)
{
	go(loop, NULL);			/* NewMux() */

	struct mux_watcher *w = &the_watcher;	/* m.Watch() */
	w->result = go_make_chan(0);
	w->stopped = go_make_chan(0);
	go_mutex_lock(&global_mtx);
	watcher = w;
	go_mutex_unlock(&global_mtx);

	watcher_stop(w);
	return NULL;
}

int main()
{
	go_init();
	go(test_mux_watcher_close, NULL);
	return 0;
}
