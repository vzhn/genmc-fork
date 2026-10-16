#include <stdbool.h>
#include <stdatomic.h>
#include <genmc.h>

/*
 * Dmitry Vyukov's bounded MPMC queue (1024cores.net), in the form of
 * crossbeam's ArrayQueue: a ring of cells, each with its own sequence
 * number. A cell with sequence pos is free for the enqueue at position pos;
 * the enqueue claims pos with a CAS, writes the data and publishes it by
 * storing pos + 1. The dequeue at pos waits for sequence pos + 1, claims pos,
 * reads the data and frees the cell for the next lap with pos + CAP.
 *
 * If the cell at pos is not published yet, the dequeue reports empty only
 * when no enqueue has claimed pos; otherwise an enqueue is in progress and
 * the dequeue waits for it (crossbeam's ArrayQueue::pop). The enqueue
 * likewise reports full only when the dequeue is a whole lap behind.
 * Position CASes are seq_cst, as in crossbeam, and use the strong CAS.
 *
 * Also included by the relinche queue client (enqueue never fails there:
 * QUEUE_CAP is at least the number of enqueues).
 *
 * One switch per bug:
 * - SPURIOUS_EMPTY: the original 1024cores dequeue, which reports empty as
 *   soon as the cell at its position is not published. If an earlier
 *   enqueue has claimed that position but not finished, a dequeue that
 *   comes after a later, finished enqueue reports empty: the queue is not
 *   linearizable (relinche reports it);
 * - BUG_RELAXED_SEQ: the dequeue loads the sequence relaxed, relying on the
 *   control dependency (a common porting mistake). The read of the data does
 *   not depend on the load, so it is not ordered after the producer's write:
 *   a data race on the cell's data.
 */

#ifndef QUEUE_CAP
# define QUEUE_CAP 4	/* a power of two */
#endif

#ifdef BUG_RELAXED_SEQ
# define DEQ_SEQ_LOAD memory_order_relaxed
#else
# define DEQ_SEQ_LOAD memory_order_acquire
#endif

struct cell {
	atomic_ulong seq;
	unsigned int data;
};

typedef struct {
	struct cell cells[QUEUE_CAP];
	atomic_ulong enq_pos;
	atomic_ulong deq_pos;
} queue_t;

void init_queue(queue_t *q, int num_threads)
{
	for (unsigned long i = 0; i < QUEUE_CAP; i++)
		atomic_init(&q->cells[i].seq, i);
	atomic_init(&q->enq_pos, 0);
	atomic_init(&q->deq_pos, 0);
}

void clear_queue(queue_t *q, int num_threads) {}

bool enqueue(queue_t *q, unsigned int data)
{
	unsigned long pos = atomic_load_explicit(&q->enq_pos, memory_order_relaxed);
	struct cell *c;

	for (;;) {
		c = &q->cells[pos % QUEUE_CAP];
		unsigned long seq = atomic_load_explicit(&c->seq, memory_order_acquire);
		long dif = (long) seq - (long) pos;

		if (dif == 0) {
			if (atomic_compare_exchange_strong_explicit(&q->enq_pos, &pos, pos + 1,
								    memory_order_seq_cst,
								    memory_order_relaxed))
				break;
			continue;
		}
		if (dif < 0) {
			/* the cell still holds the previous lap's data */
			atomic_thread_fence(memory_order_seq_cst);
			if (atomic_load_explicit(&q->deq_pos, memory_order_relaxed) + QUEUE_CAP == pos)
				return false;	/* full */
			/* a dequeue is freeing it: wait (crossbeam spins) */
			__VERIFIER_await(atomic_load_explicit(&c->seq, memory_order_relaxed) != seq ||
					 atomic_load_explicit(&q->enq_pos, memory_order_relaxed) != pos);
		}
		pos = atomic_load_explicit(&q->enq_pos, memory_order_relaxed);
	}
	c->data = data;
	atomic_store_explicit(&c->seq, pos + 1, memory_order_release);
	return true;
}

bool dequeue(queue_t *q, unsigned int *data)
{
	unsigned long pos = atomic_load_explicit(&q->deq_pos, memory_order_relaxed);
	struct cell *c;

	for (;;) {
		c = &q->cells[pos % QUEUE_CAP];
		unsigned long seq = atomic_load_explicit(&c->seq, DEQ_SEQ_LOAD);
		long dif = (long) seq - (long) (pos + 1);

		if (dif == 0) {
			if (atomic_compare_exchange_strong_explicit(&q->deq_pos, &pos, pos + 1,
								    memory_order_seq_cst,
								    memory_order_relaxed))
				break;
			continue;
		}
		if (dif < 0) {
			/* the cell is not published for this lap yet */
#ifdef SPURIOUS_EMPTY
			return false;
#else
			atomic_thread_fence(memory_order_seq_cst);
			if (atomic_load_explicit(&q->enq_pos, memory_order_relaxed) == pos)
				return false;	/* empty */
			/* an enqueue claimed pos: wait for it (crossbeam spins) */
			__VERIFIER_await(atomic_load_explicit(&c->seq, memory_order_relaxed) != seq ||
					 atomic_load_explicit(&q->deq_pos, memory_order_relaxed) != pos);
#endif
		}
		pos = atomic_load_explicit(&q->deq_pos, memory_order_relaxed);
	}
	*data = c->data;
	atomic_store_explicit(&c->seq, pos + QUEUE_CAP, memory_order_release);
	return true;
}
