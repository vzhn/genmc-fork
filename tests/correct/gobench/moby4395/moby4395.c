#include "../../../lib/go.h"

/*
 * GoBench kernel moby#4395 (blocking, channel), after Docker's utils.Go(),
 * a promise: it runs a function in a goroutine and returns a channel that
 * will carry the function's result. A caller that never receives the
 * result leaves the goroutine blocked forever on an unbuffered channel.
 *
 * BUG_UNBUFFERED_PROMISE: the result channel is unbuffered, as before moby
 * commit d3a6ee1e55a5 ("Make the chan for utils.Go buffered in order to
 * avoid goroutine leak", 2014).
 */

void *run(void *arg)
{
	go_send((go_chan_t *) arg, 1);	/* ch <- f() */
	return NULL;
}

static go_chan_t *go_promise(void)
{
#ifdef BUG_UNBUFFERED_PROMISE
	go_chan_t *ch = go_make_chan(0);
#else
	go_chan_t *ch = go_make_chan(1);
#endif
	go(run, ch);
	return ch;
}

int main()
{
	go_init();
	go_promise();		/* the result is ignored */
	return 0;
}
