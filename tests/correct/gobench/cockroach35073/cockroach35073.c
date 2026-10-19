#include "../../../lib/go.h"

/*
 * GoBench kernel cockroach#35073 (blocking, channel), after CockroachDB's
 * DistSQL outbox. Producers push rows into the outbox's RowChannel, a
 * buffered channel, as long as the consumer needs more rows; marking the
 * consumer closed makes later pushes skip the send and drains one row so
 * that a blocked producer can finish. The outbox's goroutine consumes the
 * rows, and was supposed to mark its consumer closed when it stops; when it
 * failed early, it returned without doing so.
 *
 * The test fills the buffer, starts a producer that blocks pushing one
 * more row, starts the outbox (which stops at once), waits for it and
 * pushes again. The model's buffer holds BUFROWS rows.
 *
 * BUG_NO_CONSUMER_CLOSED: the outbox does not mark its consumer closed
 * when it stops early, as before CockroachDB PR #35073 (2019), which
 * defers ConsumerClosed() for every exit.
 */

#ifndef BUFROWS
# define BUFROWS 1
#endif

#define NEED_MORE_ROWS 0
#define CONSUMER_CLOSED 2

atomic_int consumer_status;
go_chan_t *data_chan;
go_waitgroup_t wg, blocked_pusher_wg;

static void push(void)
{
	if (atomic_load(&consumer_status) == NEED_MORE_ROWS)
		go_send(data_chan, 0);
}

static void consumer_closed(void)
{
	atomic_store(&consumer_status, CONSUMER_CLOSED);
	struct go_case cs[] = { GO_RECV(data_chan, NULL) };
	go_select(cs, 1, true);
}

/* the outbox's goroutine: run() -> mainLoop() fails at once */
void *run(void *arg)
{
#ifndef BUG_NO_CONSUMER_CLOSED
	consumer_closed();
#endif
	go_wg_done(&wg);
	return NULL;
}

void *blocked_pusher(void *arg)
{
	push();
	go_wg_done(&blocked_pusher_wg);
	return NULL;
}

int main()
{
	go_init();
	data_chan = go_make_chan(BUFROWS);
	for (int i = 0; i < BUFROWS; i++)
		push();
	go_wg_add(&blocked_pusher_wg, 1);
	go(blocked_pusher, NULL);
	go_wg_add(&wg, 1);	/* outbox.start(&wg) */
	go(run, NULL);
	go_wg_wait(&wg);
	push();
	return 0;
}
