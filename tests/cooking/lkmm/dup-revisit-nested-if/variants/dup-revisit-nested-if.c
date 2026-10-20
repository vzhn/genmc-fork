#include <stdlib.h>
#include <pthread.h>
#include <lkmm.h>

/*
 * herd7: 9 executions; GenMC -lkmm explores one of them twice (10; -imm: 9;
 * vzhn.8 also 10). Found by scripts/lkmm/fuzz-ctrl.py (seed 586, reduced).
 * P0's register is a global, as litmus2c makes registers (as a local, the
 * duplicate goes away).
 * P1's read of y is revisited by P0's second write to y from two graphs
 * that differ only in where P1's write to y is in co, which P1's revisit
 * deletes: GenMC's maximality check lets both through under -lkmm (whose
 * prefix has moi), one only under -imm. In cooking until fixed.
 */

int y, z;
int r0;

void *P0(void *unused)
{
	r0 = READ_ONCE(z);
	if (r0) {
		if (r0) {
			WRITE_ONCE(y, 1);
			WRITE_ONCE(y, 2);
		} else {
			WRITE_ONCE(y, 1);
		}
	} else {
		WRITE_ONCE(y, 2);
	}
	return NULL;
}

void *P1(void *unused)
{
	(void) READ_ONCE(y);
	WRITE_ONCE(z, 2);
	WRITE_ONCE(y, 2);
	return NULL;
}

int main()
{
	pthread_t t0, t1;

	if (pthread_create(&t0, NULL, P0, NULL))
		abort();
	if (pthread_create(&t1, NULL, P1, NULL))
		abort();
	return 0;
}
