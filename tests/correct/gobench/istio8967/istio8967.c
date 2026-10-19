#include "../../../lib/go.h"

/*
 * GoBench kernel istio#8967 (non-blocking, data race), after Galley's
 * filesystem source. Start() runs a goroutine that waits for donec to
 * close; Stop() closes donec and then sets the field to nil, while the
 * goroutine may still be reading it.
 *
 * BUG_NIL_DONEC: Stop() clears donec after closing it, as before Istio PR
 * #8967 ("[galley] fix data race", 2018).
 */

struct fs_source {
	go_chan_t *donec;
};

struct fs_source s;
go_waitgroup_t wg;

void *watch(void *arg)
{
	go_recv(s.donec);
	return NULL;
}

void *test(void *arg)
{
	s.donec = go_make_chan(0);	/* New() */
	go(watch, NULL);		/* s.Start() */
	go_close(s.donec);		/* s.Stop() */
#ifdef BUG_NIL_DONEC
	s.donec = NULL;
#endif
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
