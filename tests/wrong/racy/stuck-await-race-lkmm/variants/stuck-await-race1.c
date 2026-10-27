#include <pthread.h>
#include <genmc.h>
#include <lkmm.h>

/* A race in an execution that ends with a thread spinning forever (#288) */

int x, d;

void *stuck(void *unused)
{
	while (READ_ONCE(x) != 1) /* nobody writes 1 */
		;
	return NULL;
}

void *writer(void *unused)
{
	d = 1;
	return NULL;
}

void *reader(void *unused)
{
	(void)READ_ONCE(d);
	return NULL;
}

int main()
{
	pthread_t t[3];

	pthread_create(&t[0], NULL, stuck, NULL);
	pthread_create(&t[1], NULL, writer, NULL);
	pthread_create(&t[2], NULL, reader, NULL);
	return 0;
}
