#include <stdatomic.h>
#include <stdint.h>
#include <genmc.h>

/*
 * Each await's condition holds at once, so no thread waits. The conditions
 * do arithmetic narrower than 64 bits on the loaded values: GenMC must
 * truncate and sign-extend as C does when it evaluates them, or it reports
 * a non-terminating wait under -check-liveness (args.*.in).
 */

_Atomic(uint8_t) u = 200;
_Atomic(int8_t) s = -100;
_Atomic(uint32_t) w = 7;

int main()
{
	__VERIFIER_await((uint8_t)(atomic_load(&u) + 100) == 44);	/* add wraps */
	__VERIFIER_await((uint8_t)(atomic_load(&u) - 201) == 255);	/* sub wraps */
	__VERIFIER_await((uint8_t)(atomic_load(&u) * 2) == 144);	/* mul wraps */
	__VERIFIER_await((uint8_t)(atomic_load(&u) << 1) == 144);	/* shl wraps */
	__VERIFIER_await((int32_t)atomic_load(&s) == -100);		/* sext */
	__VERIFIER_await(atomic_load(&s) < 0);				/* signed compare */
	__VERIFIER_await((int32_t)atomic_load(&s) / 3 == -33);		/* sdiv */
	__VERIFIER_await((int32_t)atomic_load(&s) >> 1 == -50);	/* ashr */
	__VERIFIER_await(atomic_load(&w) % 4 == 3);			/* urem */
	return 0;
}
