/*
 * This program is dual-licensed under the Apache License 2.0 and the MIT License.
 * You may choose to use, distribute, or modify this software under either license.
 */
/*
 * The part of the SV-COMP shim shared by svcomp-shim.h and svcomp-shim-pp.h.
 * It uses only __atomic builtins and GenMC entry points that both includers
 * declare: __VERIFIER_assume, __VERIFIER_await, __VERIFIER_thread_create.
 *
 * - __VERIFIER_nondet_*: GenMC's own __VERIFIER_nondet_int() returns the same
 *   sequence in every execution, so a task that branches on it explores one
 *   branch. Here each call reads its own choice variable, which a helper
 *   thread sets to 1, 2, ..., SVC_NONDET_MAX in turn: GenMC explores the read
 *   seeing each of 0..SVC_NONDET_MAX (default 2). Enough for branch
 *   conditions, thread counts and small indices, not for integer ranges.
 *   __VERIFIER_nondet_bool gives 0 or 1. An execution that asks for more than
 *   SVC_MAX_CHOICES values is discarded, which bounds nondet-driven loops like
 *   -unroll: a clean result only holds up to that bound.
 * - Spurious wake-ups: pthread_cond_wait may return without a signal, as
 *   POSIX allows. Each wait call asks __svc_spurious_wakeup(), which says yes
 *   in the explored executions where it reads the helper thread's 1. At most
 *   SVC_MAX_SPURIOUS (default 1) wait calls per execution may wake spuriously;
 *   later calls always wait for a signal, so a waiting loop still ends.
 * - __VERIFIER_atomic_begin/end: a global lock. Correct for atomic sections
 *   that neither nest nor block, but it differs from SV-COMP's semantics
 *   for data races in both directions. The lock adds synchronisation that
 *   uninterruptible sections do not have, so it can hide a race. And
 *   SV-COMP counts accesses inside atomic sections as atomic, so a plain
 *   access racing with one is no race there, while GenMC reports it;
 *   svcomp-run.py labels such reports ATOMIC-RACE instead of a verdict.
 * - Functions named __VERIFIER_atomic_*: SV-COMP runs them atomically.
 *   svcomp-run.py inserts SVC_ATOMIC_FUNCTION_GUARD at the top of each,
 *   which takes the atomic-section lock and releases it on every return.
 * - main(argc, argv): the shim's main passes a dummy argc/argv when
 *   svcomp-run.py defines SVC_MAIN_ARGC=1.
 */
#ifndef SVC_MAX_CHOICES
# define SVC_MAX_CHOICES 4
#endif
#ifndef SVC_NONDET_MAX
# define SVC_NONDET_MAX 2
#endif
#ifndef SVC_MAX_SPURIOUS
# define SVC_MAX_SPURIOUS 1
#endif

static int __svc_choice[SVC_MAX_CHOICES];
static int __svc_next_choice;
static int __svc_spurious[SVC_MAX_SPURIOUS > 0 ? SVC_MAX_SPURIOUS : 1];
static int __svc_next_spurious;
static int __svc_atomic_lock;

static int __svc_nondet_val(void)
{
	int i = __atomic_fetch_add(&__svc_next_choice, 1, __ATOMIC_RELAXED);
	/* out of choices: end this execution, like an unroll bound */
	__VERIFIER_assume(i < SVC_MAX_CHOICES);
	return __atomic_load_n(&__svc_choice[i], __ATOMIC_RELAXED);
}

static int __svc_nondet_bit(void)
{
	int v = __svc_nondet_val();
	/* drop the larger values instead of exploring each one as "true" */
	__VERIFIER_assume(v <= 1);
	return v;
}

int __svc_nondet_int(void) { return __svc_nondet_val(); }
unsigned int __svc_nondet_uint(void) { return __svc_nondet_val(); }
_Bool __svc_nondet_bool(void) { return __svc_nondet_bit(); }
char __svc_nondet_char(void) { return __svc_nondet_val(); }
unsigned char __svc_nondet_uchar(void) { return __svc_nondet_val(); }
short __svc_nondet_short(void) { return __svc_nondet_val(); }
unsigned short __svc_nondet_ushort(void) { return __svc_nondet_val(); }
long __svc_nondet_long(void) { return __svc_nondet_val(); }
unsigned long __svc_nondet_ulong(void) { return __svc_nondet_val(); }
void *__svc_nondet_pointer(void) { return 0; }

/* Call with the condvar's mutex held, so that the counter's order follows
 * the mutex and adds no interleavings of its own. */
static int __svc_spurious_wakeup(void)
{
	if (SVC_MAX_SPURIOUS <= 0)
		return 0;
	int i = __atomic_load_n(&__svc_next_spurious, __ATOMIC_RELAXED);
	if (i >= SVC_MAX_SPURIOUS)
		return 0;
	if (!__atomic_load_n(&__svc_spurious[i], __ATOMIC_RELAXED))
		return 0;
	__atomic_store_n(&__svc_next_spurious, i + 1, __ATOMIC_RELAXED);
	return 1;
}

void __VERIFIER_atomic_begin(void)
{
	int free = 0;
	while (!__atomic_compare_exchange_n(&__svc_atomic_lock, &free, 1, 0,
					    __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)) {
		free = 0;
		__VERIFIER_await(__atomic_load_n(&__svc_atomic_lock, __ATOMIC_RELAXED) == 0);
	}
}

void __VERIFIER_atomic_end(void)
{
	__atomic_store_n(&__svc_atomic_lock, 0, __ATOMIC_RELEASE);
}

static void __svc_atomic_guard_release(int *unused)
{
	__VERIFIER_atomic_end();
}

#define SVC_ATOMIC_FUNCTION_GUARD						\
	__attribute__((cleanup(__svc_atomic_guard_release))) int __svc_guard =	\
		(__VERIFIER_atomic_begin(), 0)

static void *__svc_chooser(void *arg)
{
	for (int i = 0; i < SVC_MAX_CHOICES; i++)
		for (int v = 1; v <= SVC_NONDET_MAX; v++)
			__atomic_store_n(&__svc_choice[i], v, __ATOMIC_RELAXED);
	for (int i = 0; i < SVC_MAX_SPURIOUS; i++)
		__atomic_store_n(&__svc_spurious[i], 1, __ATOMIC_RELAXED);
	return 0;
}

int __svcomp_main();

#undef main
int main(void)
{
	__VERIFIER_thread_create(0, __svc_chooser, 0);
#if SVC_MAIN_ARGC
	static char *argv[] = { "task", 0 };
	return __svcomp_main(1, argv);
#else
	return __svcomp_main();
#endif
}
#define main __svcomp_main
#define __VERIFIER_nondet_int __svc_nondet_int
#define __VERIFIER_nondet_uint __svc_nondet_uint
#define __VERIFIER_nondet_bool __svc_nondet_bool
#define __VERIFIER_nondet_char __svc_nondet_char
#define __VERIFIER_nondet_uchar __svc_nondet_uchar
#define __VERIFIER_nondet_short __svc_nondet_short
#define __VERIFIER_nondet_ushort __svc_nondet_ushort
#define __VERIFIER_nondet_long __svc_nondet_long
#define __VERIFIER_nondet_ulong __svc_nondet_ulong
#define __VERIFIER_nondet_pointer __svc_nondet_pointer
