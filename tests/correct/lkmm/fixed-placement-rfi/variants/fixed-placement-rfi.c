/* 2 executions: main reads 0 or 1 from f. The write of f revisits main's
 * read. Its prefix keeps the second read of x (a data dependency), not the
 * first read nor the write of 0, which depend on the branch only. The first
 * read cannot read from that write, which comes after it: the revisit must
 * not require it to read where the kept read does (issue #88). It did, and
 * every revisit was rejected as non-maximal: 1 execution. */
#include <pthread.h>
#include <lkmm.h>

int x, f;

void *t1(void *p)
{
	WRITE_ONCE(x, 1);
	if (READ_ONCE(x))
		WRITE_ONCE(x, 0);
	WRITE_ONCE(f, READ_ONCE(x) + 1);
	return NULL;
}

int main()
{
	pthread_t t;

	pthread_create(&t, NULL, t1, NULL);
	READ_ONCE(f);
	pthread_join(t, NULL);
	return 0;
}
