#include "../../../lib/go.h"

/*
 * GoBench kernel moby#4951 (blocking, AB-BA deadlock), after Docker's
 * devicemapper DeviceSet. DeleteDevice() takes the global devices lock and
 * then the device's lock; while it waits for the device to be removed, it
 * drops the global lock, sleeps and takes it again, still holding the
 * device's lock. Two goroutines delete the same device.
 *
 * While the first sleeps, the second takes the global lock and waits for
 * the device's lock; the first then waits for the global lock.
 *
 * BUG_GLOBAL_LOCK_FIRST: the global lock is taken before the device's lock,
 * as before moby commit 2ffef1b7eb61 ("devmapper: Avoid AB-BA deadlock",
 * 2014), which always takes the device's lock first.
 */

go_mutex_t devices_mu;
go_mutex_t info_lock;		/* the device's lock */

static void remove_device_and_wait(void)
{
	go_mutex_unlock(&devices_mu);
	/* time.Sleep() */
	go_mutex_lock(&devices_mu);
}

void *delete_device(void *arg)
{
#ifdef BUG_GLOBAL_LOCK_FIRST
	go_mutex_lock(&devices_mu);
	go_mutex_lock(&info_lock);
#else
	go_mutex_lock(&info_lock);
	go_mutex_lock(&devices_mu);
#endif
	remove_device_and_wait();
	go_mutex_unlock(&info_lock);
	go_mutex_unlock(&devices_mu);
	return NULL;
}

int main()
{
	go_init();
	go(delete_device, NULL);
	go(delete_device, NULL);
	return 0;
}
