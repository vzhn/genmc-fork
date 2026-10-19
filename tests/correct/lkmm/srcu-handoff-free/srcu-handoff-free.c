#include <stdlib.h>
#include <pthread.h>
#include <lkmm.h>

/*
 * A critical section that A opens (srcu_down_read()) and B closes
 * (srcu_up_read(), with the index handed over through memory), against an
 * updater C that unpublishes x, waits for a grace period and frees it.
 * A's dereference is in the section, before the handoff, so the grace
 * period orders it before the free.
 *
 * BUG_AFTER_HANDOFF: A dereferences after handing the index over; the
 * section may end before the read, and C may free x first. GenMC checks an
 * access against a free when the access is added, taking a section still
 * open as closed at the end of its thread; B's unlock may come later and
 * end it earlier, so a complete execution is checked again.
 */

struct srcu_struct s;
int *ptr;
int hand;
int idx_box;
int r;

void *A(void *unused)
{
	int idx = srcu_down_read(&s);
	int *p = srcu_dereference(ptr, &s);
#ifndef BUG_AFTER_HANDOFF
	if (p)
		r = READ_ONCE(*p);
#endif
	WRITE_ONCE(idx_box, idx);
	smp_store_release(&hand, 1);
#ifdef BUG_AFTER_HANDOFF
	if (p)
		r = READ_ONCE(*p);
#endif
	return NULL;
}

void *B(void *unused)
{
	smp_cond_load_acquire(&hand, VAL == 1);
	srcu_up_read(&s, READ_ONCE(idx_box));
	return NULL;
}

void *C(void *x)
{
	WRITE_ONCE(ptr, NULL);
	synchronize_srcu(&s);
	free(x);
	return NULL;
}

int main()
{
	pthread_t ta, tb, tc;
	int *x = malloc(sizeof(*x));

	*x = 1;
	ptr = x;
	if (pthread_create(&ta, NULL, A, NULL))
		abort();
	if (pthread_create(&tb, NULL, B, NULL))
		abort();
	if (pthread_create(&tc, NULL, C, x))
		abort();
	if (pthread_join(ta, NULL))
		abort();
	if (pthread_join(tb, NULL))
		abort();
	if (pthread_join(tc, NULL))
		abort();
	return 0;
}
