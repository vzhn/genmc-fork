#include <pthread.h>
#include <assert.h>

/*
 * A stack variable's fields are named in error traces (loc.a, loc.b), as a
 * global's are. With LLVM's debug records, the declarations of stack
 * variables were not found, and their accesses printed as (, 0)
 */

struct pair {
	int a;
	int b;
};

void *thread(void *arg)
{
	struct pair *p = arg;
	p->b = 1;
	return NULL;
}

int main()
{
	struct pair loc = {0, 0};
	pthread_t t;

	pthread_create(&t, NULL, thread, &loc);
	assert(loc.b == 0);
	pthread_join(t, NULL);
	return 0;
}
