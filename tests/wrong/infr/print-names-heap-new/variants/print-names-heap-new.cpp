#include <pthread.h>
#include <cassert>

/*
 * A block from new is named after it (#144), and the blocks of one call
 * are numbered
 */

struct S {
	int a;
	int b;
};

S *gs[2];

void *thread(void *)
{
	S *s = new S;
	s->b = 1;
	for (int i = 0; i < 2; i++) {
		gs[i] = new S;
		gs[i]->a = i;
	}
	assert(0);
	return nullptr;
}

int main()
{
	pthread_t t;

	pthread_create(&t, nullptr, thread, nullptr);
	return 0;
}
