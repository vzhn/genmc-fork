#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>

/*
 * The block list behind tokio's mpsc channel (sync/mpsc/list.rs, block.rs).
 * Messages go into a linked list of blocks of BLOCK_CAP slots. A sender
 * claims a slot index with a fetch_add on tail_position, walks the list from
 * block_tail to the block holding that index (growing the list if needed)
 * and writes the slot. A sender walking past a block whose slots are all
 * written may advance block_tail past it; it then records the current tail
 * position in the block (tx_release). The single receiver reads slots in
 * order and frees a block it has passed only once its index has reached the
 * recorded tail position: every sender that might still be walking through
 * the block has claimed an index below that position, so has written it.
 *
 * Indices wrap around (usize in tokio). To reach the wrap with a few
 * messages, indices here are 8 bits and the channel starts just below the
 * wrap, at index 252. Sender F sends three messages, sender S one, and the
 * receiver R receives up to four; main receives the rest and frees the
 * list. Every message is received exactly once. The model frees a
 * reclaimed block (tokio first tries to reuse it at the end of the list).
 *
 * BUG_WRAPPING_COMPARE: tokio before the fix of issue #8545 (PR #8546)
 * compared the recorded tail position and the receiver's index with
 * "required_index > self.index". When the tail has wrapped (position 0) and
 * the receiver has not (index 254), the block is freed while the sender
 * that claimed index 255, having loaded block_tail before it advanced, still
 * walks through it: a use-after-free. The fix compares the wrapping
 * difference as a signed number.
 */

#define BLOCK_CAP 2
#define READY_MASK ((1u << BLOCK_CAP) - 1)
#define RELEASED (1u << BLOCK_CAP)
#define START 252

typedef uint8_t idx_t;

struct block {
	idx_t start_index;		/* set before the block is published */
	_Atomic(struct block *) next;
	atomic_uint ready_slots;	/* written slots, RELEASED */
	idx_t observed_tail_position;	/* written before RELEASED */
	int values[BLOCK_CAP];
};

/* Tx (shared by the senders) */
atomic_uint tail_position;		/* used modulo 256 */
_Atomic(struct block *) block_tail;

/* Rx (the receiver only) */
struct block *head;
struct block *free_head;
idx_t rx_index;

atomic_int received[5];

static idx_t start_index(idx_t i) { return i & ~(BLOCK_CAP - 1); }
static unsigned offset(idx_t i) { return i & (BLOCK_CAP - 1); }

static struct block *new_block(idx_t start)
{
	struct block *b = calloc(1, sizeof(*b));

	b->start_index = start;
	return b;
}

/* Appends a block after b, or returns the one another sender appended */
static struct block *grow(struct block *b)
{
	struct block *new = new_block(b->start_index + BLOCK_CAP);
	struct block *next = NULL;

	if (atomic_compare_exchange_strong_explicit(&b->next, &next, new, memory_order_acq_rel,
						    memory_order_acquire))
		return new;
	free(new);
	return next;
}

static bool is_final(struct block *b)
{
	return (atomic_load_explicit(&b->ready_slots, memory_order_acquire) & READY_MASK) ==
	       READY_MASK;
}

static void tx_release(struct block *b, idx_t tail)
{
	b->observed_tail_position = tail;
	atomic_fetch_or_explicit(&b->ready_slots, RELEASED, memory_order_release);
}

static struct block *find_block(idx_t slot)
{
	idx_t start = start_index(slot);
	struct block *b = atomic_load_explicit(&block_tail, memory_order_acquire);
	idx_t distance = (idx_t) (start - b->start_index) / BLOCK_CAP;
	bool try_updating_tail = distance > offset(slot);

	for (;;) {
		if (b->start_index == start)
			return b;

		struct block *next = atomic_load_explicit(&b->next, memory_order_acquire);

		if (next == NULL)
			next = grow(b);
		try_updating_tail = try_updating_tail && is_final(b);
		if (try_updating_tail) {
			struct block *expected = b;

			if (atomic_compare_exchange_strong_explicit(&block_tail, &expected, next,
								    memory_order_release,
								    memory_order_relaxed)) {
				idx_t tail = atomic_fetch_add_explicit(&tail_position, 0,
								       memory_order_release);
				tx_release(b, tail);
			} else {
				try_updating_tail = false;
			}
		}
		b = next;
	}
}

static void send(int value)
{
	idx_t slot = atomic_fetch_add_explicit(&tail_position, 1, memory_order_acquire);
	struct block *b = find_block(slot);

	b->values[offset(slot)] = value;
	atomic_fetch_or_explicit(&b->ready_slots, 1u << offset(slot), memory_order_release);
}

static bool try_advancing_head(void)
{
	idx_t start = start_index(rx_index);

	for (;;) {
		if (head->start_index == start)
			return true;

		struct block *next = atomic_load_explicit(&head->next, memory_order_acquire);

		if (next == NULL)
			return false;
		head = next;
	}
}

static void reclaim_blocks(void)
{
	while (free_head != head) {
		struct block *b = free_head;

		if (!(atomic_load_explicit(&b->ready_slots, memory_order_acquire) & RELEASED))
			return;

		idx_t required = b->observed_tail_position;

#ifdef BUG_WRAPPING_COMPARE
		if (required > rx_index)
			return;
#else
		if ((int8_t) (idx_t) (required - rx_index) > 0)
			return;
#endif
		free_head = atomic_load_explicit(&b->next, memory_order_relaxed);
		free(b);
	}
}

/* Receives the next message, if it is there */
static bool recv(int *value)
{
	if (!try_advancing_head())
		return false;
	reclaim_blocks();

	unsigned ready = atomic_load_explicit(&head->ready_slots, memory_order_acquire);

	if (!(ready & (1u << offset(rx_index))))
		return false;
	*value = head->values[offset(rx_index)];
	rx_index++;
	return true;
}

static void got(int value)
{
	assert(value >= 1 && value <= 4);
	int before = atomic_fetch_add_explicit(&received[value], 1, memory_order_relaxed);
	assert(before == 0);
}

static void *sender_f(void *unused)
{
	send(1);
	send(2);
	send(3);
	return NULL;
}

static void *sender_s(void *unused)
{
	send(4);
	return NULL;
}

static void *receiver(void *unused)
{
	int value;

	for (int i = 0; i < 4 && recv(&value); i++)
		got(value);
	return NULL;
}

int main()
{
	pthread_t f, s, r;
	int value;

	head = free_head = new_block(START);
	rx_index = START;
	atomic_init(&block_tail, head);
	atomic_init(&tail_position, START);

	if (pthread_create(&f, NULL, sender_f, NULL))
		abort();
	if (pthread_create(&s, NULL, sender_s, NULL))
		abort();
	if (pthread_create(&r, NULL, receiver, NULL))
		abort();
	if (pthread_join(f, NULL))
		abort();
	if (pthread_join(s, NULL))
		abort();
	if (pthread_join(r, NULL))
		abort();

	while (recv(&value))
		got(value);
	for (int i = 1; i <= 4; i++)
		assert(received[i] == 1);
	while (free_head) {
		struct block *b = free_head;

		free_head = atomic_load_explicit(&b->next, memory_order_relaxed);
		free(b);
	}
	return 0;
}
