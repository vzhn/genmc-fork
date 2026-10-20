/* 5 executions, as under rc11. U hands a job to A once W has published x
 * through y; A acknowledges it, W then completes, and M may observe that.
 * The hand-off revisits A's read of slot and keeps W's second write of x
 * (the source of b, which is in U's prefix through y), while W re-executes
 * a = READ_ONCE(x) into the hole before it. The read cannot read the
 * po-later write, but it was taken as not added maximally, against the
 * graph's co-maximal write (issue #100), so the revisit of M's read by
 * W's completion, which deletes it, was rejected: 4 executions, M never
 * reading done = 1. */
#include <pthread.h>
#include <lkmm.h>

static int x, y, slot, ack, done;
static int r1, r2, a, b;

static void *M(void *arg)
{
	r2 = READ_ONCE(done);
	return NULL;
}

static void *A(void *arg)
{
	r1 = READ_ONCE(slot);
	if (r1)
		WRITE_ONCE(ack, 1);
	return NULL;
}

static void *W(void *arg)
{
	WRITE_ONCE(x, 1);
	a = READ_ONCE(x);
	WRITE_ONCE(x, 2);
	b = READ_ONCE(x);
	WRITE_ONCE(y, b);
	if (READ_ONCE(ack))
		WRITE_ONCE(done, 1);
	return NULL;
}

static void *U(void *arg)
{
	if (READ_ONCE(y) == 2)
		WRITE_ONCE(slot, 1);
	return NULL;
}

int main(void)
{
	pthread_t pm, pa, pw, pu;
	pthread_create(&pm, NULL, M, NULL);
	pthread_create(&pa, NULL, A, NULL);
	pthread_create(&pw, NULL, W, NULL);
	pthread_create(&pu, NULL, U, NULL);
	pthread_join(pm, NULL);
	pthread_join(pa, NULL);
	pthread_join(pw, NULL);
	pthread_join(pu, NULL);
	return 0;
}
