#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>

/*
 * The __VERIFIER_nondet_X() of SV-COMP, as genmc.h declares them, return a
 * value of their type (the same one in every execution). The sum keeps the
 * conversions of the values. A writer and a reader of x give two executions
 * with calls in three threads.
 */

long long sum;
atomic_uint x;

void *writer(void *arg)
{
	atomic_store_explicit(&x, __VERIFIER_nondet_uint() | 1, memory_order_relaxed);
	return NULL;
}

void *reader(void *arg)
{
	sum += atomic_load_explicit(&x, memory_order_relaxed) + __VERIFIER_nondet_short();
	return NULL;
}

int main()
{
	pthread_t tw, tr;

	bool b = __VERIFIER_nondet_bool();
	assert(b == 0 || b == 1);

	sum += b;
	sum += __VERIFIER_nondet_char();
	sum += __VERIFIER_nondet_uchar();
	sum += __VERIFIER_nondet_short();
	sum += __VERIFIER_nondet_ushort();
	sum += __VERIFIER_nondet_int();
	sum += __VERIFIER_nondet_uint();
	sum += __VERIFIER_nondet_unsigned();
	sum += __VERIFIER_nondet_long();
	sum += __VERIFIER_nondet_ulong();
	sum += __VERIFIER_nondet_longlong();
	sum += __VERIFIER_nondet_ulonglong();
	sum += __VERIFIER_nondet_size_t();
#ifdef __SIZEOF_INT128__
	sum += __VERIFIER_nondet_int128();
	sum += __VERIFIER_nondet_uint128();
#endif

	pthread_create(&tw, NULL, writer, NULL);
	pthread_create(&tr, NULL, reader, NULL);
	pthread_join(tw, NULL);
	pthread_join(tr, NULL);
	return 0;
}
