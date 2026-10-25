/* -lkmm: a store through a poisoned pointer while another thread is in an
 * RCU read-side critical section. The store is checked before it is placed
 * in coherence, and an error in a graph with RCU is checked with the open
 * sections closed (#201): that check evaluated the model with the unplaced
 * store, and GenMC crashed walking its coherence predecessors.
 */
#include <lkmm.h>
#include <pthread.h>
#include <stddef.h>

int x;

void *reader(void *u)
{
	rcu_read_lock();
	READ_ONCE(x);
	rcu_read_unlock();
	return NULL;
}

void *writer(void *u)
{
	int **p = (int **) 0x122;
	WRITE_ONCE(*p, &x);
	return NULL;
}

int main()
{
	pthread_t t1, t2;
	pthread_create(&t1, NULL, reader, NULL);
	pthread_create(&t2, NULL, writer, NULL);
	return 0;
}
