#include <stdlib.h>
#include <lkmm.h>
#include <pthread.h>
#include <assert.h>

/*
 * Two plain writes, ordered by a release and an acquire: the later one
 * must be later in coherence (ww-incoh), as the reads after the joins see.
 * GenMC's coherence view is po and thread synchronisation alone (LKMM's
 * coherence is per-location SC), so only plain-coherence forbids the
 * other order. There is no race.
 */

int x;
int y;
int r1;

void *P0(void *unused)
{
	x = 1;
	smp_store_release(&y, 1);
	return NULL;
}

void *P1(void *unused)
{
	r1 = smp_load_acquire(&y);
	if (r1)
		x = 2;
	return NULL;
}

int main()
{
	pthread_t t0, t1;

	pthread_create(&t0, NULL, P0, NULL);
	pthread_create(&t1, NULL, P1, NULL);
	pthread_join(t0, NULL);
	pthread_join(t1, NULL);

	assert(x == (r1 ? 2 : 1));
	return 0;
}
