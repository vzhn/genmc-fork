#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <stddef.h>

/*
 * An SV-COMP task declares the __VERIFIER_nondet_X() it calls itself, with
 * SV-COMP's types: they agree with genmc.h's (pthread.h includes it), and
 * the ones genmc.h leaves out (u32, loff_t, sector_t) work too. As in
 * nondet-variants0.c, a writer and a reader give two executions.
 */

extern _Bool __VERIFIER_nondet_bool(void);
extern char __VERIFIER_nondet_char(void);
extern unsigned char __VERIFIER_nondet_uchar(void);
extern short __VERIFIER_nondet_short(void);
extern unsigned short __VERIFIER_nondet_ushort(void);
extern int __VERIFIER_nondet_int(void);
extern unsigned int __VERIFIER_nondet_uint(void);
extern unsigned __VERIFIER_nondet_unsigned(void);
extern long __VERIFIER_nondet_long(void);
extern unsigned long __VERIFIER_nondet_ulong(void);
extern long long __VERIFIER_nondet_longlong(void);
extern unsigned long long __VERIFIER_nondet_ulonglong(void);
extern size_t __VERIFIER_nondet_size_t(void);
extern unsigned int __VERIFIER_nondet_u32(void);
extern long long __VERIFIER_nondet_loff_t(void);
extern unsigned long long __VERIFIER_nondet_sector_t(void);

long long sum;
atomic_uint x;

void *writer(void *arg)
{
	atomic_store_explicit(&x, __VERIFIER_nondet_u32() | 1, memory_order_relaxed);
	return NULL;
}

void *reader(void *arg)
{
	sum += atomic_load_explicit(&x, memory_order_relaxed) + __VERIFIER_nondet_char();
	return NULL;
}

int main()
{
	pthread_t tw, tr;

	_Bool b = __VERIFIER_nondet_bool();
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
	sum += __VERIFIER_nondet_loff_t();
	sum += __VERIFIER_nondet_sector_t();

	pthread_create(&tw, NULL, writer, NULL);
	pthread_create(&tr, NULL, reader, NULL);
	pthread_join(tw, NULL);
	pthread_join(tr, NULL);
	return 0;
}
