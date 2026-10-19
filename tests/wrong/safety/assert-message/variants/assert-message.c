#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>

/* The report of a failed assertion includes the asserted expression */

atomic_int balance = 1;

void *withdraw(void *unused)
{
	if (atomic_load(&balance) > 0)
		atomic_fetch_sub(&balance, 1);
	return NULL;
}

int main()
{
	pthread_t t1, t2;

	pthread_create(&t1, NULL, withdraw, NULL);
	pthread_create(&t2, NULL, withdraw, NULL);
	pthread_join(t1, NULL);
	pthread_join(t2, NULL);
	assert(balance >= 0);
	return 0;
}
