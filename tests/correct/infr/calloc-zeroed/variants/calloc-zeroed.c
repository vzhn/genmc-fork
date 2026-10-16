#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>

/*
 * calloc() initializes its block in one event: readers that are hb-after the
 * allocation read zeros without any store having been recorded, and the
 * block's bytes do not appear in the graph.
 */

struct blk {
	int a[4];
	long b;
};

static struct blk *p;
static atomic_int sum;

void *reader(void *arg)
{
	int i = (int)(uintptr_t)arg;

	assert(p->a[i] == 0 && p->b == 0);
	atomic_fetch_add(&sum, p->a[i] + 1);
	return NULL;
}

void *writer(void *arg)
{
	p->a[3] = 7;
	return NULL;
}

int main()
{
	pthread_t t[3];

	p = calloc(1, sizeof(*p));
	assert(p->a[0] == 0 && p->b == 0);

	pthread_create(&t[0], NULL, reader, (void *)0);
	pthread_create(&t[1], NULL, reader, (void *)1);
	pthread_create(&t[2], NULL, writer, NULL);
	for (int i = 0; i < 3; i++)
		pthread_join(t[i], NULL);

	assert(atomic_load(&sum) == 2 && p->a[3] == 7);
	free(p);
	return 0;
}
