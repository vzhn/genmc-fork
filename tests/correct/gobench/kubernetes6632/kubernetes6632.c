#include "../../../lib/go.h"

/*
 * GoBench kernel kubernetes#6632 (blocking, channel and lock), after the
 * idleAwareFramer of docker/spdystream, vendored in Kubernetes. Every frame
 * written signals resetChan, holding writeLock. When the connection closes,
 * monitor() takes writeLock, closes resetChan and sets it to nil, so that
 * later writes skip the signal.
 *
 * Nothing receives from resetChan once monitor() has stopped watching it, so
 * a write that is blocked sending on it holds writeLock forever, and
 * monitor() cannot take the lock to close the channel.
 *
 * BUG_NO_DRAIN: monitor() takes the lock without draining resetChan, as
 * before docker/spdystream 99515db39d3d (pulled into Kubernetes by commit
 * 82afb7ab1fe1, 2015), which first starts a goroutine that drains it.
 */

go_chan_t *close_chan;
go_mutex_t write_lock;
go_chan_t *reset_chan_field;	/* i.resetChan, under write_lock */

void *drain(void *arg)
{
	bool ok;

	do
		go_recv2((go_chan_t *) arg, &ok);
	while (ok);
	return NULL;
}

/* G1 */
void *monitor(void *arg)
{
	go_chan_t *reset_chan = arg;

	go_recv(close_chan);
#ifndef BUG_NO_DRAIN
	go(drain, reset_chan);
#endif
	go_mutex_lock(&write_lock);
	go_close(reset_chan);
	reset_chan_field = NULL;
	go_mutex_unlock(&write_lock);
	return NULL;
}

/* G2 */
void *write_frame(void *arg)
{
	go_mutex_lock(&write_lock);
	if (reset_chan_field)
		go_send(reset_chan_field, 1);
	go_mutex_unlock(&write_lock);
	return NULL;
}

void *helper(void *arg)
{
	go_send(close_chan, 1);
	return NULL;
}

int main()
{
	go_init();
	reset_chan_field = go_make_chan(0);
	close_chan = go_make_chan(0);
	go(helper, NULL);
	go(monitor, reset_chan_field);
	go(write_frame, NULL);
	return 0;
}
