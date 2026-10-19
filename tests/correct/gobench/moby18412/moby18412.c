#include "../../../lib/go.h"

/*
 * GoBench kernel moby#18412 (non-blocking, order violation), after Docker's
 * integration-cli RunCommandWithOutputForDuration(). A goroutine waits for
 * the command, stores its exit code in the caller's result variable and
 * reports on an unbuffered done channel; the caller waits for done or a
 * timeout. On a timeout the caller returns its results, reading the exit
 * code the goroutine may be writing.
 *
 * BUG_SHARED_EXIT_CODE: the goroutine writes the caller's exitCode, as
 * before moby PR #18412 ("Fix race in RunCommandWithOutputForDuration",
 * 2015), which sends the exit code through a buffered channel instead.
 */

int exit_code;			/* the named result */
go_chan_t *done;

void *wait_cmd(void *arg)
{
	/* exitErr := cmd.Wait() */
#ifdef BUG_SHARED_EXIT_CODE
	exit_code = 1;
	go_send(done, 0);
#else
	go_send(done, 1);	/* exitInfo{exitErr, exitCode} */
#endif
	return NULL;
}

int main()
{
	go_init();
#ifdef BUG_SHARED_EXIT_CODE
	done = go_make_chan(0);
#else
	done = go_make_chan(1);
#endif
	go(wait_cmd, NULL);		/* cmd.Start() */

	long info;
	struct go_case cs[] = { GO_RECV(go_time_after(), NULL), GO_RECV(done, &info) };
	if (go_select(cs, 2, false) == 1) {
#ifndef BUG_SHARED_EXIT_CODE
		exit_code = info;
#endif
	}
	assert(exit_code == 0 || exit_code == 1);	/* return output, exitCode, ... */
	return 0;
}
