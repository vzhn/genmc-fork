#include <assert.h>
#include <pthread.h>
#include <stdint.h>

/*
 * Thread-local aggregates (issue #90): each thread has its own copy of a
 * struct with an array field (with and without an initializer) and of an
 * array, starting as the initializer. A field at a non-zero offset is a
 * location of its own, and no thread sees another thread's writes.
 */

struct node {
	int x;
	int a[2];
	long y;
};

static __thread struct node tl = {1, {2, 3}, 4};
static __thread struct node zero;
static _Thread_local int arr[3] = {5, 6, 7};

void *thread(void *arg)
{
	int id = (int)(intptr_t)arg;

	assert(tl.x == 1 && tl.a[0] == 2 && tl.a[1] == 3 && tl.y == 4);
	assert(zero.x == 0 && zero.a[0] == 0 && zero.a[1] == 0 && zero.y == 0);
	assert(arr[0] == 5 && arr[1] == 6 && arr[2] == 7);

	tl.a[1] = id;
	zero.y = id;
	arr[1] = id;

	assert(tl.x == 1 && tl.a[0] == 2 && tl.a[1] == id && tl.y == 4);
	assert(zero.x == 0 && zero.a[1] == 0 && zero.y == id);
	assert(arr[0] == 5 && arr[1] == id && arr[2] == 7);
	return NULL;
}

int main()
{
	pthread_t t1, t2;

	tl.a[1] = 42;
	arr[2] = 42;

	pthread_create(&t1, NULL, thread, (void *)1);
	pthread_create(&t2, NULL, thread, (void *)2);
	pthread_join(t1, NULL);
	pthread_join(t2, NULL);

	assert(tl.x == 1 && tl.a[1] == 42 && arr[1] == 6 && arr[2] == 42);
	assert(zero.y == 0);
	return 0;
}
