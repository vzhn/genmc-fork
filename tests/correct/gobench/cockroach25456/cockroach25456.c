#include "../../../lib/go.h"

/*
 * GoBench kernel cockroach#25456 (blocking, channel), after CockroachDB's
 * consistency queue. After a failed consistency check, process() asks
 * whether the server is shutting down by receiving from the stopper's
 * quiesce channel, which only closes at shutdown; outside shutdown the
 * receive blocks. The test processes a replica twice without stopping.
 *
 * BUG_BLOCKING_QUIESCE_CHECK: process() receives from the quiesce channel,
 * as before CockroachDB PR #25456 (2018), which polls it with a select and
 * a default case.
 */

go_chan_t *quiescer;

static void process(void)
{
#ifdef BUG_BLOCKING_QUIESCE_CHECK
	go_recv(quiescer);
#else
	struct go_case cs[] = { GO_RECV(quiescer, NULL) };
	go_select(cs, 1, true);
#endif
}

int main()
{
	go_init();
	quiescer = go_make_chan(0);
	for (int i = 0; i < 2; i++)
		process();
	return 0;
}
