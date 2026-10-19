#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <assert.h>
#include <genmc.h>
#include <lkmm.h>

/*
 * Userspace RCU, memory-barrier flavor (liburcu's urcu-mb), written with the
 * kernel's primitives: tests/correct/classic/urcu with smp_mb(),
 * READ_ONCE()/WRITE_ONCE() (liburcu's CMM_LOAD_SHARED/CMM_STORE_SHARED),
 * rcu_assign_pointer() and rcu_dereference(). Under -lkmm this checks that
 * the implementation gives RCU's guarantee under the kernel's memory model:
 * the writer frees an object only after every reader that could have found
 * it has left its read-side section. The functions are urcu_*(), as
 * <lkmm.h> has the kernel's own RCU.
 *
 * BUG_SINGLE_FLIP: urcu_synchronize() flips the phase and waits only once,
 * so a reader that stored a stale phase is skipped and its object freed.
 */

#define GP_COUNT 1UL
#define GP_PHASE (1UL << 32)
#define NEST_MASK (GP_PHASE - 1)

struct object {
	int value;
};

unsigned long gp_ctr = GP_COUNT;
unsigned long reader_ctr;
struct object *shared;

static void urcu_read_lock(void)
{
	unsigned long tmp = READ_ONCE(reader_ctr);

	if (!(tmp & NEST_MASK)) {
		WRITE_ONCE(reader_ctr, READ_ONCE(gp_ctr));
		smp_mb();
	} else {
		WRITE_ONCE(reader_ctr, tmp + GP_COUNT);
	}
}

static void urcu_read_unlock(void)
{
	smp_mb();
	WRITE_ONCE(reader_ctr, READ_ONCE(reader_ctr) - GP_COUNT);
}

/* Is the reader inside a read-side section begun in the other phase? */
static bool gp_ongoing(void)
{
	unsigned long v = READ_ONCE(reader_ctr);

	return (v & NEST_MASK) && ((v ^ READ_ONCE(gp_ctr)) & ~NEST_MASK);
}

static void update_counter_and_wait(void)
{
	WRITE_ONCE(gp_ctr, READ_ONCE(gp_ctr) ^ GP_PHASE);
	smp_mb();
	__VERIFIER_await(!gp_ongoing());
}

static void urcu_synchronize(void)
{
	smp_mb();
	update_counter_and_wait();
#ifndef BUG_SINGLE_FLIP
	smp_mb();
	update_counter_and_wait();
#endif
	smp_mb();
}

static struct object *new_object(int v)
{
	struct object *o = malloc(sizeof(*o));

	o->value = v;
	return o;
}

static void *writer(void *unused)
{
	for (int v = 1; v <= 2; v++) {
		struct object *old = READ_ONCE(shared);

		rcu_assign_pointer(shared, new_object(v));
		urcu_synchronize();
		free(old);
	}
	return NULL;
}

static void *reader(void *unused)
{
	urcu_read_lock();
	struct object *o = rcu_dereference(shared);
	int v = o->value;
	urcu_read_unlock();
	assert(v >= 0 && v <= 2);
	return NULL;
}

int main()
{
	pthread_t w, r;

	shared = new_object(0);
	if (pthread_create(&w, NULL, writer, NULL))
		abort();
	if (pthread_create(&r, NULL, reader, NULL))
		abort();
	if (pthread_join(w, NULL))
		abort();
	if (pthread_join(r, NULL))
		abort();
	free(READ_ONCE(shared));
	return 0;
}
