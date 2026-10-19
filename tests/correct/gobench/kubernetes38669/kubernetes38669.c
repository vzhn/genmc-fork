#include "../../../lib/go.h"

/*
 * GoBench kernel kubernetes#38669 (blocking, channel), after the
 * apiserver's cacheWatcher. A watcher's goroutine first sends the initial
 * events on its result channel, then forwards input until input closes;
 * Stop() closes input. Here nobody reads the results: the watch is
 * stopped right away.
 *
 * The goroutine then blocks forever sending the first initial event.
 *
 * BUG_RESULT_SEND_IGNORES_STOP: sendWatchCacheEvent() sends on result
 * unconditionally, as before Kubernetes PR #38669 (2016), which adds a
 * done channel, closed by stop(), and selects between the two.
 */

#define NINIT 2

struct cache_watcher {
	go_mutex_t mu;
	go_chan_t *input;
	go_chan_t *result;
	go_chan_t *done;	/* the fix */
	bool stopped;
};

struct cache_watcher w;

static void stop(void)
{
	go_mutex_lock(&w.mu);
	if (!w.stopped) {
		w.stopped = true;
		go_close(w.input);
#ifndef BUG_RESULT_SEND_IGNORES_STOP
		go_close(w.done);
#endif
	}
	go_mutex_unlock(&w.mu);
}

static void send_watch_cache_event(long event)
{
#ifdef BUG_RESULT_SEND_IGNORES_STOP
	go_send(w.result, event);
#else
	struct go_case cs[] = { GO_SEND(w.result, event), GO_RECV(w.done, NULL) };
	go_select(cs, 2, false);
#endif
}

void *process(void *arg)
{
	for (int i = 1; i <= NINIT; i++)
		send_watch_cache_event(i);
	for (;;) {
		bool ok;

		go_recv2(w.input, &ok);
		if (!ok)
			break;
	}
	stop();
	go_close(w.result);
	return NULL;
}

int main()
{
	go_init();
	w.input = go_make_chan(0);
	w.result = go_make_chan(0);
	w.done = go_make_chan(0);
	go(process, NULL);	/* newCacheWatcher() */
	stop();
	return 0;
}
