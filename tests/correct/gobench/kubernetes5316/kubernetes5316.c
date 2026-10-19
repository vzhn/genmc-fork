#include "../../../lib/go.h"

/*
 * GoBench kernel kubernetes#5316 (blocking, channel), after the apiserver's
 * finishRequest(). It runs the request in a goroutine, which reports the
 * result on ch or the error on errCh, and waits for either or for a
 * timeout. If the timeout wins, nobody receives the goroutine's report.
 * In the model the request fails (the kernel picks at random).
 *
 * BUG_UNBUFFERED_RESULT: ch and errCh are unbuffered, as before Kubernetes
 * commit cc3a433a7abc ("fix goroutine leak", 2015), which buffers them.
 */

#ifdef BUG_UNBUFFERED_RESULT
# define RESULT_CAP 0
#else
# define RESULT_CAP 1
#endif

go_chan_t *ch, *err_ch;

/* G2: the request either succeeds or fails */
void *request(void *arg)
{
	if ((long) arg)
		go_send(err_ch, 1);
	else
		go_send(ch, 1);
	return NULL;
}

/* G1 */
void *finish_request(void *arg)
{
	ch = go_make_chan(RESULT_CAP);
	err_ch = go_make_chan(RESULT_CAP);
	go(request, arg);

	struct go_case cs[] = { GO_RECV(ch, NULL), GO_RECV(err_ch, NULL),
				GO_RECV(go_time_after(), NULL) };
	go_select(cs, 3, false);
	return NULL;
}

int main()
{
	go_init();
	go(finish_request, (void *) 1);
	return 0;
}
