#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>

/*
 * KVM's vCPU requests (kvm_make_request()/kvm_check_request() in
 * include/linux/kvm_host.h). A requester fills in the request's data and
 * sets a bit in vcpu->requests; the vCPU tests the bit, clears it and acts
 * on the data. Kernel primitives map to C11: set_bit(), test_bit() and
 * clear_bit() are relaxed atomic operations, smp_wmb() a release fence
 * and smp_mb__after_atomic() a seq_cst fence.
 *
 * One requester publishes a payload and sets the bit; the vCPU, if it sees
 * the bit, must see the payload.
 *
 * BUG_NO_BARRIERS: no smp_wmb() before set_bit() and no
 * smp_mb__after_atomic() after clear_bit(), as before commit 2e4682ba2ed7
 * ("KVM: add missing memory barrier in kvm_{make,check}_request", 2016).
 * The vCPU can see the bit and still read the old payload. Passes under
 * SC.
 */

#define REQ_BIT 1UL

atomic_ulong requests;
atomic_int payload;

static void kvm_make_request(void)
{
#ifndef BUG_NO_BARRIERS
	atomic_thread_fence(memory_order_release);		/* smp_wmb() */
#endif
	atomic_fetch_or_explicit(&requests, REQ_BIT, memory_order_relaxed);
}

static bool kvm_check_request(void)
{
	if (!(atomic_load_explicit(&requests, memory_order_relaxed) & REQ_BIT))
		return false;
	atomic_fetch_and_explicit(&requests, ~REQ_BIT, memory_order_relaxed);
#ifndef BUG_NO_BARRIERS
	atomic_thread_fence(memory_order_seq_cst);		/* smp_mb__after_atomic() */
#endif
	return true;
}

static void *requester(void *arg)
{
	atomic_store_explicit(&payload, 42, memory_order_relaxed);
	kvm_make_request();
	return NULL;
}

static void *vcpu(void *arg)
{
	if (kvm_check_request())
		assert(atomic_load_explicit(&payload, memory_order_relaxed) == 42);
	return NULL;
}

int main()
{
	pthread_t r, v;

	if (pthread_create(&r, NULL, requester, NULL) ||
	    pthread_create(&v, NULL, vcpu, NULL))
		abort();
	if (pthread_join(r, NULL) || pthread_join(v, NULL))
		abort();
	return 0;
}
