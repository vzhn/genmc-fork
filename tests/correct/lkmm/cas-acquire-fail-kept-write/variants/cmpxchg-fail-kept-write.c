#include <pthread.h>
#include <lkmm.h>

/*
 * Issue #230 with a fully ordered cmpxchg(): its accesses carry the Mb
 * attribute only when it succeeds (a failed cmpxchg() orders nothing), so
 * the store after it is ordered after it only on success, as with the C11
 * compare-exchange of cas-acquire-fail-kept-write.c (herd7: 6 executions).
 */

int v0, v1;
int a1, a2, b1, b2;

void *t1(void *unused)
{
	a1 = READ_ONCE(v0);
	a2 = READ_ONCE(v0);
	WRITE_ONCE(v1, 1);
	return NULL;
}

void *t2(void *unused)
{
	b1 = READ_ONCE(v0);
	b2 = cmpxchg(&v1, 0, 1);
	WRITE_ONCE(v0, 1);
	return NULL;
}

int main()
{
	pthread_t a, b;

	pthread_create(&a, NULL, t1, NULL);
	pthread_create(&b, NULL, t2, NULL);
	return 0;
}
