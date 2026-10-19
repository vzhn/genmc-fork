#include "../../../lib/go.h"

/*
 * GoBench kernel kubernetes#26980 (blocking, channel and lock), after
 * client-go's shared informer processorListener. add() appends a
 * notification under the listener's lock and broadcasts its condition;
 * pop() waits on the condition for a notification and then blocks handing
 * it on until the stop channel closes. Another goroutine takes the lock;
 * the test waits for it, and closes the stop channel only on return.
 *
 * pop() blocked while still holding the lock, so the other goroutine, the
 * test, and with it pop() itself, all wait forever.
 *
 * BUG_POP_BLOCKS_HOLDING_LOCK: pop() blocks with the lock held, as before
 * Kubernetes PR #26980 (2016), which hands the notification on after
 * releasing it.
 */

go_mutex_t lock;
go_cond_t cond;
int pending_notifications;
go_chan_t *stop_ch;
go_chan_t *result_ch;

static void add(void)
{
	go_mutex_lock(&lock);
	pending_notifications++;
	go_cond_broadcast(&cond);
	go_mutex_unlock(&lock);
}

void *pop(void *arg)
{
	go_mutex_lock(&lock);
	while (pending_notifications == 0) {
		struct go_case cs[] = { GO_RECV(stop_ch, NULL) };
		if (go_select(cs, 1, true) == 0) {
			go_mutex_unlock(&lock);
			return NULL;
		}
		go_cond_wait(&cond, &lock);
	}
	pending_notifications--;
#ifndef BUG_POP_BLOCKS_HOLDING_LOCK
	go_mutex_unlock(&lock);
#endif
	go_recv(stop_ch);	/* hand the notification on, until stopped */
#ifdef BUG_POP_BLOCKS_HOLDING_LOCK
	go_mutex_unlock(&lock);
#endif
	return NULL;
}

void *locker(void *arg)
{
	go_mutex_lock(&lock);
	go_close(result_ch);
	return NULL;
}

int main()
{
	go_init();
	stop_ch = go_make_chan(0);
	result_ch = go_make_chan(0);
	add();
	go(pop, NULL);
	go(locker, NULL);
	go_recv(result_ch);
	go_mutex_unlock(&lock);
	go_close(stop_ch);	/* defer close(stopCh) */
	return 0;
}
