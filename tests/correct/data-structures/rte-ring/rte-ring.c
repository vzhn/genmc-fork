#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>

/*
 * DPDK's rte_ring, C11 version (rte_ring_c11_pvt.h), multi-producer and
 * multi-consumer, one object per call. Producers and consumers each have a
 * head and a tail. An enqueue claims a slot by moving prod.head with a CAS
 * after checking cons.tail for room, writes the object, waits until
 * prod.tail reaches its old head (earlier enqueues are done) and publishes
 * by storing prod.tail with release. A dequeue does the same on cons.head,
 * checking prod.tail for objects. The ring slots are plain memory.
 *
 * Without a switch, the orderings of DPDK since 2025: the head load is
 * acquire and the head CAS release (acquire on failure), the other side's
 * tail load acquire. The wait in update_tail() is acquire too, which DPDK's
 * is not (BUG_RELAXED_WAIT).
 *
 * Thread X enqueues 1 and then dequeues, thread Z enqueues 2 and thread Y
 * dequeues; main drains the ring afterwards. Every object comes out exactly
 * once. Before the threads start, main enqueues and dequeues once, so the
 * threads' slots wrap.
 *
 * One switch per bug:
 * - BUG_RELAXED_TAIL: the C11 port as first merged (commit 39368ebfc6) read
 *   the other side's tail with a plain load; fixed in 2018 ("ring: fix c11
 *   memory ordering issue") with load-acquire, pairing with the release in
 *   update_tail(). Without it a consumer reads a slot that is not ordered
 *   after the producer's write: a data race;
 * - BUG_PARTIAL_ORDER: DPDK from 2018 to 2025: a relaxed head load, an
 *   acquire fence, then the tail load, and a relaxed head CAS. A consumer can
 *   read the head that another consumer's CAS just moved, yet a tail older
 *   than the one that consumer saw: the count of entries underflows and it
 *   dequeues a slot nobody produced (found on Arm with RCpc acquire; fixed
 *   by "ring: safe partial ordering for head/tail update", 2025);
 * - BUG_RELAXED_WAIT: DPDK's update_tail() waits for the earlier
 *   enqueue with a relaxed load and then stores the tail with release. A
 *   consumer that acquires this tail synchronizes with this producer only,
 *   not with the earlier one whose tail it waited for, so in C11 its read of
 *   the earlier producer's slot races with the write. (Arm and POWER release
 *   stores order the loads before them too, so the hardware is not
 *   affected; the C11 model is.)
 */

#define SIZE 4			/* a power of two */
#define MASK (SIZE - 1)
#define CAPACITY MASK

#if defined(BUG_RELAXED_TAIL)
# define HEAD_LOAD memory_order_acquire
# define TAIL_LOAD memory_order_relaxed
# define CAS_SUCC memory_order_acquire
# define CAS_FAIL memory_order_relaxed
#elif defined(BUG_PARTIAL_ORDER)
# define HEAD_LOAD memory_order_relaxed
# define HEAD_FENCE
# define TAIL_LOAD memory_order_acquire
# define CAS_SUCC memory_order_relaxed
# define CAS_FAIL memory_order_relaxed
#else
# define HEAD_LOAD memory_order_acquire
# define TAIL_LOAD memory_order_acquire
# define CAS_SUCC memory_order_release
# define CAS_FAIL memory_order_acquire
#endif

#ifdef BUG_RELAXED_WAIT
# define WAIT_LOAD memory_order_relaxed
#else
# define WAIT_LOAD memory_order_acquire
#endif

struct headtail {
	atomic_uint head;
	atomic_uint tail;
};

struct ring {
	struct headtail prod;
	struct headtail cons;
	int slots[SIZE];
};

struct ring r;
atomic_int seen[3];

static void update_tail(struct headtail *ht, uint32_t old_val, uint32_t new_val)
{
	/* wait for the enqueues (dequeues) that claimed earlier slots */
	__VERIFIER_await(atomic_load_explicit(&ht->tail, WAIT_LOAD) == old_val);
	atomic_store_explicit(&ht->tail, new_val, memory_order_release);
}

/*
 * Claims one slot on d's side, checking s's tail (the other side); capacity
 * is CAPACITY for producers and 0 for consumers. Returns false if there is
 * no room (no object).
 */
static bool move_head(struct headtail *d, struct headtail *s, uint32_t capacity,
		      uint32_t *old_head)
{
	*old_head = atomic_load_explicit(&d->head, HEAD_LOAD);
	for (;;) {
#ifdef HEAD_FENCE
		/* read the head before the tail */
		atomic_thread_fence(memory_order_acquire);
#endif
		uint32_t stail = atomic_load_explicit(&s->tail, TAIL_LOAD);
		uint32_t entries = capacity + stail - *old_head;

		if (entries < 1)
			return false;
		/* on failure, *old_head is updated */
		if (atomic_compare_exchange_strong_explicit(&d->head, old_head, *old_head + 1,
							    CAS_SUCC, CAS_FAIL))
			return true;
	}
}

static bool enqueue(int obj)
{
	uint32_t head;

	if (!move_head(&r.prod, &r.cons, CAPACITY, &head))
		return false;
	r.slots[head & MASK] = obj;
	update_tail(&r.prod, head, head + 1);
	return true;
}

static bool dequeue(int *obj)
{
	uint32_t head;

	if (!move_head(&r.cons, &r.prod, 0, &head))
		return false;
	*obj = r.slots[head & MASK];
	update_tail(&r.cons, head, head + 1);
	return true;
}

static void got(int obj)
{
	assert(obj == 1 || obj == 2);	/* an object that was enqueued */
	int before = atomic_fetch_add_explicit(&seen[obj], 1, memory_order_relaxed);
	assert(before == 0);
}

static void *thread_x(void *unused)
{
	int obj;
	bool ok = enqueue(1);

	assert(ok);
	if (dequeue(&obj))
		got(obj);
	return NULL;
}

static void *thread_y(void *unused)
{
	int obj;

	if (dequeue(&obj))
		got(obj);
	return NULL;
}

static void *thread_z(void *unused)
{
	bool ok = enqueue(2);

	assert(ok);
	return NULL;
}

int main()
{
	pthread_t x, y, z;
	int obj;

	enqueue(0);
	dequeue(&obj);

	if (pthread_create(&x, NULL, thread_x, NULL))
		abort();
	if (pthread_create(&y, NULL, thread_y, NULL))
		abort();
	if (pthread_create(&z, NULL, thread_z, NULL))
		abort();
	if (pthread_join(x, NULL))
		abort();
	if (pthread_join(y, NULL))
		abort();
	if (pthread_join(z, NULL))
		abort();

	while (dequeue(&obj))
		got(obj);
	assert(seen[1] == 1 && seen[2] == 1);
	return 0;
}
