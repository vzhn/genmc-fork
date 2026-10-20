#include <pthread.h>

/* Two symmetric threads, joined in the opposite order of their spawns:
 * after joining b, main reads data while a may still write it. Symmetry
 * reduction explores a no later than b, so it missed the race */

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
int data;

void *thread(void *unused)
{
	pthread_mutex_lock(&m);
	data = 1;
	pthread_mutex_unlock(&m);
	return NULL;
}

int main()
{
	pthread_t a, b;

	pthread_create(&a, NULL, thread, NULL);
	pthread_create(&b, NULL, thread, NULL);
	pthread_join(b, NULL);
	int r = data;
	pthread_join(a, NULL);
	return r;
}
