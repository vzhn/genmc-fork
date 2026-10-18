/*
 * The MPMC queue with its loops bounded by -unroll instead of spin-assume,
 * so that threads get killed when they exceed the bound. In the execution
 * where main is killed in a weak-CAS retry loop, the unfreed-memory warning
 * for the pthread_t arrays starts an error replay whose target is the kill
 * itself; the driver used to fail an internal check when the replay ended
 * at the kill (see kill-error-replay for the minimal case).
 */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "../../mpmc-queue/mpmc-queue.c"

int main()
{
	pthread_t W[MAXWRITERS], R[MAXREADERS], RW[MAXRDWR];

//	printf("%d reader(s), %d writer(s)\n", readers, writers);

#ifndef CONFIG_MPMC_NO_INITIAL_ELEMENT
//	printf("Adding initial element\n");
	int32_t *bin = write_prepare(&queue);
//	store_32(bin, 17);
	*bin = 17;
	write_publish(&queue);
#endif

//	printf("Start threads\n");

	for (int i = 0; i < writers; i++)
		pthread_create(&W[i], NULL, threadW, NULL);
	for (int i = 0; i < readers; i++)
		pthread_create(&R[i], NULL, threadR, NULL);
	for (int i = 0; i < rdwr; i++)
		pthread_create(&RW[i], NULL, threadRW, NULL);

	for (int i = 0; i < writers; i++)
		pthread_join(W[i], NULL);
	for (int i = 0; i < readers; i++)
		pthread_join(R[i], NULL);
	for (int i = 0; i < rdwr; i++)
		pthread_join(RW[i], NULL);

	/* printf("Threads ended\n"); */

	return 0;
}
