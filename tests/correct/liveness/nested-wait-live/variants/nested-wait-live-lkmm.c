/* The same program written with the LKMM primitives */
#include <pthread.h>
#include <lkmm.h>
#include <genmc.h>

spinlock_t l;
int state = 2, gen;

void *waiter(void *unused)
{
	for (;;) {
		int s;
		spin_lock(&l);
		s = state;
		spin_unlock(&l);
		if (s != 2)
			break;
		for (;;) {
			int st, g;
			spin_lock(&l);
			st = state;
			g = gen;
			spin_unlock(&l);
			if (st != 2)
				break;
			__VERIFIER_await(READ_ONCE(gen) != g);
		}
	}
	return NULL;
}

void *stopper(void *unused)
{
	spin_lock(&l);
	WRITE_ONCE(state, 0);
	WRITE_ONCE(gen, gen + 1);
	spin_unlock(&l);
	return NULL;
}

int main()
{
	pthread_t t1, t2;

	pthread_create(&t1, NULL, waiter, NULL);
	pthread_create(&t2, NULL, stopper, NULL);
	return 0;
}
