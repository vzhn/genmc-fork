#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>

/*
 * Epoch-based reclamation as in Concurrency Kit's ck_epoch (ck_epoch.c,
 * ck_epoch.h). A reader marks its record active and copies the global
 * epoch into it (ck_epoch_begin), then clears active (ck_epoch_end). A
 * writer that unlinked an object calls ck_epoch_synchronize() before
 * freeing it: it scans the records and, while some active reader has not
 * seen the current epoch, waits for it; after a scan it advances the
 * epoch, up to two times (grace period of three epochs). If a whole scan
 * finds no active reader, nobody can hold the object and it stops early.
 *
 * Two readers: L entered its read section in epoch 0 and is still in it
 * when the threads start (its record is set up so); R enters now, in epoch
 * 1, and reads the object if it is still linked. The writer W unlinks the
 * object, synchronizes and frees it. R is scanned before L. Waits are
 * awaits; ck's fences are C11 fences.
 *
 * One switch per bug:
 * - BUG_IDLE_SHORTCUT: ck before 0.6.0, where "an optimization ... to allow
 *   synchronize to succeed on an idle system ... was incorrect" (ck 0.6.0
 *   release notes). When a scan resumed after waiting for a lagging reader,
 *   the active flag was recomputed from the records still to scan only. W
 *   finds R active (but current) and L lagging, waits for L; L leaves, the
 *   resumed scan sees no active reader and W returns without waiting for R,
 *   frees the object and R reads freed memory. The fix counts a resumed scan
 *   as having seen an active reader, so it short-circuits only when all
 *   readers are idle;
 * - BUG_NO_BEGIN_FENCE: ck_epoch_begin() without the fence between setting
 *   active and loading the epoch, which its comment says is necessary
 *   ("store to load serialization"). R's store of active and W's scan
 *   form store buffering: W can miss R's active flag while R misses the
 *   unlink, so W frees the object R is reading. Passes under SC.
 */

#define GRACE 3
#define NREC 2		/* record 0: R, record 1: L */

struct record {
	atomic_uint active;
	atomic_uint epoch;
};

struct object {
	int value;
};

atomic_uint global_epoch = 1;
struct record rec[NREC];
_Atomic(struct object *) shared;

static void epoch_begin(struct record *r)
{
	atomic_store_explicit(&r->active, 1, memory_order_relaxed);
#ifndef BUG_NO_BEGIN_FENCE
	atomic_thread_fence(memory_order_seq_cst);
#endif
	atomic_store_explicit(&r->epoch, atomic_load_explicit(&global_epoch, memory_order_relaxed),
			      memory_order_relaxed);
}

static void epoch_end(struct record *r)
{
	atomic_thread_fence(memory_order_release);
	atomic_store_explicit(&r->active, 0, memory_order_relaxed);
}

/*
 * Returns the first record from i on that is active in an epoch other than
 * epoch, or -1; *af tells whether an active record was seen.
 */
static int scan(int cr, unsigned epoch, bool *af)
{
	int i;

	if (cr < 0) {
		i = 0;
		*af = false;
	} else {
		/* resume at the lagging record */
		i = cr;
#ifdef BUG_IDLE_SHORTCUT
		*af = false;
#else
		*af = true;
#endif
	}
	for (; i < NREC; i++) {
		unsigned active = atomic_load_explicit(&rec[i].active, memory_order_relaxed);

		*af |= active != 0;
		if (active && atomic_load_explicit(&rec[i].epoch, memory_order_relaxed) != epoch)
			return i;
	}
	return -1;
}

static void synchronize(void)
{
	atomic_thread_fence(memory_order_seq_cst);

	unsigned epoch = atomic_load_explicit(&global_epoch, memory_order_relaxed);
	unsigned delta = epoch, goal = epoch + GRACE;
	bool active;

	for (int i = 0; i < GRACE - 1; i++) {
		int cr = -1;

		while ((cr = scan(cr, delta, &active)) >= 0) {
			unsigned e_d = atomic_load_explicit(&global_epoch, memory_order_relaxed);

			if (e_d == delta) {
				/* stall until the lagging reader moves on */
				__VERIFIER_await(
					atomic_load_explicit(&rec[cr].active, memory_order_relaxed) == 0 ||
					atomic_load_explicit(&rec[cr].epoch, memory_order_relaxed) == delta ||
					atomic_load_explicit(&global_epoch, memory_order_relaxed) != delta);
				continue;
			}
			/* another writer advanced the epoch */
			delta = e_d;
			if (goal > epoch && delta >= goal)
				goto leave;
			cr = -1;
		}

		/* no active reader: nobody can hold the object */
		if (!active)
			break;

		unsigned seen = delta;

		if (atomic_compare_exchange_strong_explicit(&global_epoch, &seen, delta + 1,
							    memory_order_seq_cst,
							    memory_order_seq_cst))
			delta = delta + 1;
		else
			delta = seen;
		atomic_thread_fence(memory_order_acquire);
	}
leave:
	atomic_thread_fence(memory_order_seq_cst);
}

static void *reader_r(void *unused)
{
	epoch_begin(&rec[0]);

	struct object *o = atomic_load_explicit(&shared, memory_order_acquire);

	if (o != NULL)
		assert(o->value == 42);
	epoch_end(&rec[0]);
	return NULL;
}

static void *reader_l(void *unused)
{
	/* in its read section since epoch 0; leaves now */
	epoch_end(&rec[1]);
	return NULL;
}

static void *writer(void *unused)
{
	struct object *o = atomic_exchange_explicit(&shared, NULL, memory_order_relaxed);

	synchronize();
	free(o);
	return NULL;
}

int main()
{
	pthread_t r, l, w;
	struct object *o = malloc(sizeof(*o));

	o->value = 42;
	atomic_init(&shared, o);
	atomic_init(&rec[1].active, 1);
	atomic_init(&rec[1].epoch, 0);

	if (pthread_create(&r, NULL, reader_r, NULL))
		abort();
	if (pthread_create(&l, NULL, reader_l, NULL))
		abort();
	if (pthread_create(&w, NULL, writer, NULL))
		abort();
	if (pthread_join(r, NULL))
		abort();
	if (pthread_join(l, NULL))
		abort();
	if (pthread_join(w, NULL))
		abort();
	return 0;
}
