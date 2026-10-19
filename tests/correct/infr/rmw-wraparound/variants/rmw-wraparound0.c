#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>
#include <assert.h>
#include <genmc.h>

/*
 * The results of fetch_add, fetch_sub and fetch_nand wrap around at the
 * width of the access. GenMC used to keep them as 64-bit values (1 + -1 on
 * an int became 2^32), which a plain load hides by truncating, but which a
 * CAS comparing against the expected value or an await condition sees:
 * the CAS failed and the await blocked forever.
 */

/* two threads decrement; the last write value is computed on a revisit */
atomic_int shared = 2;

void *decrement(void *unused)
{
	atomic_fetch_add(&shared, -1);
	return NULL;
}

void check_cas_after_wrap(void)
{
	atomic_int x = 1;
	atomic_fetch_add(&x, -1);	/* 0 */
	int expected = 0;
	assert(atomic_compare_exchange_strong(&x, &expected, 5));
	assert(atomic_load(&x) == 5);

	_Atomic(uint8_t) c = 255;
	assert(atomic_fetch_add(&c, 1) == 255);	/* 0 */
	uint8_t ec = 0;
	assert(atomic_compare_exchange_strong(&c, &ec, 9));

	_Atomic(int16_t) s = -32768;
	atomic_fetch_sub(&s, 1);	/* 32767 */
	int16_t es = 32767;
	assert(atomic_compare_exchange_strong(&s, &es, 1));

	_Atomic(uint32_t) u = 0;
	atomic_fetch_sub(&u, 1);	/* 0xffffffff */
	uint32_t eu = 0xffffffff;
	while (!atomic_compare_exchange_weak(&u, &eu, 3))
		eu = 0xffffffff;
}

void check_await_after_wrap(void)
{
	_Atomic(uint8_t) c = 200;
	atomic_fetch_add(&c, 100);	/* 44 */
	__VERIFIER_await(atomic_load(&c) == 44);

	_Atomic(uint16_t) s = 10;
	atomic_fetch_sub(&s, 20);	/* 65526 */
	__VERIFIER_await(atomic_load(&s) == 65526);
}

void check_nand(void)
{
	_Atomic(uint32_t) n = 0xf0f0f0f0;
	__atomic_fetch_nand((uint32_t *)&n, 0xffff0000, __ATOMIC_SEQ_CST);
	uint32_t expected = 0x0f0fffff;
	assert(atomic_compare_exchange_strong(&n, &expected, 0));

	_Atomic(int8_t) m = -1;
	__atomic_fetch_nand((int8_t *)&m, (int8_t)-1, __ATOMIC_SEQ_CST);	/* 0 */
	int8_t em = 0;
	assert(atomic_compare_exchange_strong(&m, &em, 1));
}

void check_min_after_wrap(void)
{
	_Atomic(int8_t) i = 100;
	atomic_fetch_add(&i, 100);	/* -56 */
	__atomic_fetch_min((int8_t *)&i, -50, __ATOMIC_SEQ_CST);
	int8_t ei = -56;
	assert(atomic_compare_exchange_strong(&i, &ei, 0));
}

void check_wide(void)
{
	_Atomic(uint64_t) w = 1;
	atomic_fetch_sub(&w, 2);	/* 2^64 - 1 */
	uint64_t expected = UINT64_MAX;
	assert(atomic_compare_exchange_strong(&w, &expected, 0));

	_Atomic(__int128) q = 5;
	atomic_fetch_sub(&q, 2);
	__int128 eq = 3;
	assert(atomic_compare_exchange_strong(&q, &eq, 0));
}

int main()
{
	pthread_t a, b;

	pthread_create(&a, NULL, decrement, NULL);
	pthread_create(&b, NULL, decrement, NULL);

	check_cas_after_wrap();
	check_await_after_wrap();
	check_nand();
	check_min_after_wrap();
	check_wide();

	pthread_join(a, NULL);
	pthread_join(b, NULL);
	int e = 0;
	assert(atomic_compare_exchange_strong(&shared, &e, 7));
	return 0;
}
