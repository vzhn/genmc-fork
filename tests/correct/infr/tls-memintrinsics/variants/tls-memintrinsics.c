#include <assert.h>
#include <pthread.h>
#include <string.h>

/*
 * memset() and memcpy() on thread-locals: clang addresses a thread-local
 * through @llvm.threadlocal.address, which hides the variable's type, so
 * these were not split into accesses per field (and GenMC aborted with
 * "Invalid call to memset()!")
 */

struct node {
	int x;
	int a[2];
	long y;
};

static __thread struct node tl = {1, {2, 3}, 4};
static __thread struct node other;
static __thread int arr[4];

void *thread(void *unused)
{
	memset(&tl, 0, sizeof(tl));
	assert(tl.x == 0 && tl.a[0] == 0 && tl.a[1] == 0 && tl.y == 0);

	other.x = 5;
	other.a[1] = 7;
	other.y = 8;
	memcpy(&tl, &other, sizeof(tl));
	assert(tl.x == 5 && tl.a[0] == 0 && tl.a[1] == 7 && tl.y == 8);

	memset(&arr[1], 0xff, 2 * sizeof(int));
	assert(arr[0] == 0 && arr[1] == -1 && arr[2] == -1 && arr[3] == 0);
	return NULL;
}

int main()
{
	pthread_t t;

	pthread_create(&t, NULL, thread, NULL);
	pthread_join(t, NULL);
	assert(tl.x == 1 && tl.a[1] == 3 && tl.y == 4 && arr[1] == 0);
	return 0;
}
