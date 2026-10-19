#include "../../../lib/go.h"

/*
 * GoBench kernel kubernetes#70892 (non-blocking, data race), after the
 * scheduler's PrioritizeNodes(). The nodes are scored in parallel with
 * workqueue.ParallelizeUntil(); each node's work also ran the old-style
 * priority functions, filling results[i] for every priority the first time
 * it saw them empty, so two workers could fill the same results[i].
 *
 * BUG_RESULTS_PER_NODE: every node's work fills the priority results, as
 * before Kubernetes PR #70892 ("Fix a race in the scheduler", 2018), which
 * computes them in a separate parallel pass, one priority per piece.
 */

#define NPRIORITIES 2
#define NPIECES 2
#define NWORKERS 2

int results[NPRIORITIES];	/* results[i][0] */
go_chan_t *to_process;
go_waitgroup_t wg;

static void do_work_piece(int piece)
{
#ifdef BUG_RESULTS_PER_NODE
	for (int i = 0; i < NPRIORITIES; i++)
		if (results[i] != 4)
			results[i] = 7;
#else
	if (results[piece] != 4)
		results[piece] = 7;
#endif
}

void *worker(void *arg)
{
	for (;;) {
		bool ok;
		long piece = go_recv2(to_process, &ok);

		if (!ok)
			break;
		do_work_piece(piece);
	}
	go_wg_done(&wg);
	return NULL;
}

/* workqueue.ParallelizeUntil(ctx, NWORKERS, NPIECES, doWorkPiece) */
int main()
{
	go_init();
	to_process = go_make_chan(NPIECES);
	for (int i = 0; i < NPIECES; i++)
		go_send(to_process, i);
	go_close(to_process);
	go_wg_add(&wg, NWORKERS);
	for (int i = 0; i < NWORKERS; i++)
		go(worker, NULL);
	go_wg_wait(&wg);
	return 0;
}
