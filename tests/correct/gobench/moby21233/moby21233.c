#include "../../../lib/go.h"

/*
 * GoBench kernel moby#21233 (blocking, channel), after Docker's
 * distribution/xfer TestTransfer. Each transfer's watcher goroutine writes
 * progress updates to an unbuffered progress channel until the transfer is
 * released; Release() closes the watcher's releaseChan and waits for the
 * watcher to stop. The test consumes the progress channel in a goroutine.
 *
 * The consumer checked every update and called t.Fatalf on an unexpected
 * one, which ends the consuming goroutine: the watchers then block writing
 * progress, Release() waits for them, and the test hangs instead of
 * failing. Here the consumer meets such an update at once, and the test
 * runs one transfer.
 *
 * BUG_CONSUMER_EXITS: the consumer stops consuming on a failed check, as
 * before moby commit 2f4aa9658408 ("Fix flaky test TestTransfer", 2016),
 * which records the failure and keeps draining the channel.
 */

#define NTRANSFERS 1

struct watcher {
	go_chan_t *release_chan;
	go_chan_t *running;
};

go_chan_t *progress_chan;
go_chan_t *progress_done;
struct watcher watchers[NTRANSFERS];

/* G2: the watcher goroutine of Transfer.Watch() */
void *watch(void *arg)
{
	struct watcher *w = arg;
	bool done = false;

	for (;;) {
		go_send(progress_chan, 1);	/* progressOutput.WriteProgress() */
		if (done)
			break;
		go_recv(w->release_chan);	/* select { signalChan, releaseChan } */
		done = true;
	}
	go_close(w->running);
	return NULL;
}

/* G3: the progress consumer */
void *consume(void *arg)
{
	for (;;) {
		bool ok;

		go_recv2(progress_chan, &ok);
		if (!ok)
			break;
#ifdef BUG_CONSUMER_EXITS
		return NULL;		/* t.Fatalf() */
#endif
	}
	go_close(progress_done);
	return NULL;
}

static void release(struct watcher *w)
{
	go_close(w->release_chan);
	go_recv(w->running);
}

int main()
{
	go_init();
	progress_chan = go_make_chan(0);
	progress_done = go_make_chan(0);
	go(consume, NULL);
	for (int i = 0; i < NTRANSFERS; i++) {
		watchers[i].release_chan = go_make_chan(0);
		watchers[i].running = go_make_chan(0);
		go(watch, &watchers[i]);
	}
	for (int i = 0; i < NTRANSFERS; i++)
		release(&watchers[i]);
	go_close(progress_chan);
	go_recv(progress_done);
	return 0;
}
