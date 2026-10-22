/*
 * A thread's copy of a thread-local ends with the thread (issue #90):
 * thread 1 publishes the address of its copy and ends, and another thread
 * writes through it, concurrently with the end of thread 1 (variant 0) or
 * after joining it (variant 1)
 */

static __thread int tl;
_Atomic(int *) p;

void *thread1(void *unused)
{
	tl = 1;
	p = &tl;
	return NULL;
}

void *thread2(void *unused)
{
	int *q = p;
	if (q != NULL)
		*q = 42;
	return NULL;
}
