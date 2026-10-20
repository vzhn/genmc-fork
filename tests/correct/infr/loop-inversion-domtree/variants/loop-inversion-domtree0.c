#include <pthread.h>

/* Loop jump threading inverts this loop (the flag starts at 0, so the first
 * iteration always runs). The spinloop pass then used a stale dominator
 * tree and built a broken module ("Instruction does not dominate all
 * uses") */

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
int b;

void *waiter(void *unused)
{
	for (int r = 0; !r;) {
		pthread_mutex_lock(&m);
		r = b > 0;
		pthread_mutex_unlock(&m);
	}
	return NULL;
}

void *setter(void *unused)
{
	pthread_mutex_lock(&m);
	b = 1;
	pthread_mutex_unlock(&m);
	return NULL;
}

int main()
{
	pthread_t w, s;

	pthread_create(&w, NULL, waiter, NULL);
	pthread_create(&s, NULL, setter, NULL);
	pthread_join(w, NULL);
	pthread_join(s, NULL);
	return 0;
}
