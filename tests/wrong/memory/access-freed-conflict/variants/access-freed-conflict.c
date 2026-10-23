/* An access after the block is freed: the error names the free (#199; see
 * double-free-conflict).
 */
#include <pthread.h>
#include <stdlib.h>

int *p;

void *t1(void *arg)
{
	free(p);
	return NULL;
}

int main(void)
{
	pthread_t a;

	p = malloc(sizeof(int));
	pthread_create(&a, NULL, t1, NULL);
	pthread_join(a, NULL);
	*p = 1;
	return 0;
}
