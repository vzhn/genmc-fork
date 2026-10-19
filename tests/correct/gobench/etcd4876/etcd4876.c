#include "../../../lib/go.h"

/*
 * GoBench kernel etcd#4876 (non-blocking, data race), after etcd's v3rpc
 * watch server. A watch stream's send loop reads the package variable
 * ProgressReportInterval to start its ticker; a test sets the variable to
 * speed itself up while a watch is starting.
 *
 * BUG_PLAIN_INTERVAL: the interval is a plain variable, as before etcd PR
 * #4876 (2016), which makes it an atomic int32.
 */

#ifdef BUG_PLAIN_INTERVAL
long progress_report_interval = 10;
# define LOAD_INTERVAL() (progress_report_interval)
# define STORE_INTERVAL(v) (progress_report_interval = (v))
#else
atomic_long progress_report_interval = 10;
# define LOAD_INTERVAL() atomic_load(&progress_report_interval)
# define STORE_INTERVAL(v) atomic_store(&progress_report_interval, (v))
#endif

go_waitgroup_t wg;

/* sws.sendLoop() */
void *send_loop(void *arg)
{
	long interval = LOAD_INTERVAL();	/* time.NewTicker(ProgressReportInterval) */
	assert(interval > 0);
	return NULL;
}

void *set_interval(void *arg)
{
	STORE_INTERVAL(3);
	go_wg_done(&wg);
	return NULL;
}

void *watch(void *arg)
{
	go(send_loop, NULL);		/* w.Watch() */
	go_wg_done(&wg);
	return NULL;
}

void *test(void *arg)
{
	go(set_interval, NULL);
	go(watch, NULL);
	go_wg_done(&wg);
	return NULL;
}

int main()
{
	go_init();
	go_wg_add(&wg, 3);
	go(test, NULL);
	go_wg_wait(&wg);
	return 0;
}
