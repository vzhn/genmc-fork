#include <stdlib.h>
#include <pthread.h>
#include <assert.h>

/*
 * A heap block is named after the call that allocated it (#144), with the
 * fields of the type its result is stored as: a local, a global or a
 * function's result. A larger block is an array of that type; a block of
 * unknown type shows offsets.
 */

struct node {
	int key;
	struct node *next;
};

struct node *g;

static struct node *mk(void)
{
	return malloc(sizeof(struct node));
}

void *thread(void *unused)
{
	struct node *a = malloc(sizeof(*a));
	struct node *b = mk();
	struct node *arr = calloc(3, sizeof(struct node));
	char *buf = malloc(8);
	void *raw = malloc(16);

	for (int i = 0; i < 2; i++)
		g = calloc(1, sizeof(*g));
	a->key = 1;
	b->next = NULL;
	arr[2].next = NULL;
	buf[5] = 'x';
	((int *)raw)[2] = 7;
	g->key = 2;
	assert(0);
	return NULL;
}

int main()
{
	pthread_t t;

	pthread_create(&t, NULL, thread, NULL);
	return 0;
}
