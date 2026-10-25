/* -lkmm: t2's write revisits t1's read of x and keeps t1's write to z,
 * which t2's loop read reads (herd7 allows r == 1). On the new path t1
 * writes y before the kept write, so the graph is checked when the run
 * ends or reports an error, and the store through a poisoned pointer is
 * reported before it is placed in coherence: that check evaluated the model
 * with the unplaced store, and GenMC crashed walking its coherence
 * predecessors.
 */
#include <lkmm.h>
#include <pthread.h>
#include <stddef.h>

int x, y, z;

void *t1(void *u)
{
	int r = READ_ONCE(x);

	if (r)
		WRITE_ONCE(y, 1);
	WRITE_ONCE(z, 1);
	if (r)
		WRITE_ONCE(*(int *) 0x122, 1);
	return NULL;
}

void *t2(void *u)
{
	while (READ_ONCE(z) != 1)
		;
	WRITE_ONCE(x, 1);
	return NULL;
}

int main()
{
	pthread_t a, b;
	pthread_create(&a, NULL, t1, NULL);
	pthread_create(&b, NULL, t2, NULL);
	return 0;
}
