#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>
#include "../../../lib/lock.h"

/*
 * Money transfer between two accounts, each guarded by its own lock (Java
 * Concurrency in Practice, Listings 10.2 and 10.3). One thread moves 10 from
 * account 0 to 1, another moves 20 from 1 to 0. A transfer holds both locks,
 * so no money is lost or created.
 *
 * Without a switch both locks are taken in a fixed order (account id).
 *
 * BUG_LOCK_ORDER: the locks are taken in the order of the transfer (from,
 * then to). Each thread can take its first lock and wait forever for the
 * other's: deadlock (-check-liveness).
 */

struct account {
	lock_t lock;
	int balance;		/* guarded by lock */
};

struct account accounts[2] = { { 0, 100 }, { 0, 100 } };

static void transfer(int from, int to, int amount)
{
#ifdef BUG_LOCK_ORDER
	int first = from, second = to;
#else
	int first = from < to ? from : to;
	int second = from < to ? to : from;
#endif

	lock_acquire(&accounts[first].lock);
	lock_acquire(&accounts[second].lock);
	accounts[from].balance -= amount;
	accounts[to].balance += amount;
	lock_release(&accounts[second].lock);
	lock_release(&accounts[first].lock);
}

static void *thread_0to1(void *unused)
{
	transfer(0, 1, 10);
	return NULL;
}

static void *thread_1to0(void *unused)
{
	transfer(1, 0, 20);
	return NULL;
}

int main()
{
	pthread_t a, b;

	if (pthread_create(&a, NULL, thread_0to1, NULL))
		abort();
	if (pthread_create(&b, NULL, thread_1to0, NULL))
		abort();
	if (pthread_join(a, NULL))
		abort();
	if (pthread_join(b, NULL))
		abort();
	assert(accounts[0].balance == 110 && accounts[1].balance == 90);
	return 0;
}
