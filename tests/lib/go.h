#ifndef GOBENCH_GO_H
#define GOBENCH_GO_H
/*
 * A model of the Go runtime pieces that the GoBench kernels use, for porting
 * Go concurrency bugs to GenMC: goroutines, channels and select, sync.Mutex,
 * sync.RWMutex, sync.WaitGroup, sync.Cond, sync.Once, timers and context
 * cancellation.
 *
 * A Go blocking bug leaves goroutines blocked forever. Every blocking
 * operation here waits in __VERIFIER_await(), so with -check-liveness GenMC
 * reports such a goroutine as a liveness violation, whether main is stuck
 * too (a deadlock) or has returned (a goroutine leak). A panic is a failed
 * assertion. Data races are GenMC's own non-atomic races.
 *
 * Channels follow the Go runtime (runtime/chan.go, runtime/select.go): a
 * goroutine that must wait parks a record (a sudog) on the channel's send or
 * receive queue, and the goroutine that completes the operation fills in
 * the record and wakes it. A select parks one record on every case's
 * channel; the first operation to reach one of them fires the select, and
 * the others are skipped. One scheduler lock protects all channel state, so
 * a select sees its cases atomically, as Go's lockall() does. When several
 * cases are ready, Go picks one at random; here the choice is explored, for
 * the first GO_MAX_CHOICES such selects in an execution (later ones take
 * the first ready case).
 *
 * sync.RWMutex is Go's algorithm (sync/rwmutex.go): a writer that is waiting
 * blocks new readers. sync.Cond is cond.h, whose tickets match Go's
 * notifyList. Go's sync/atomic operations are sequentially consistent, so
 * the model uses seq_cst atomics. Timers fire at an arbitrary point: a
 * timer is a goroutine that sends on the timer's channel.
 *
 * Call go_init() first in main. Channels, contexts and timers come from
 * fixed pools (GO_MAX_CHANS, GO_MAX_CTXS), so nothing is freed.
 */
#include <stdbool.h>
#include <stdatomic.h>
#include <pthread.h>
#include <assert.h>
#include <genmc.h>
#include "lock.h"
#include "sem.h"
#include "cond.h"

#ifndef GO_MAX_CHOICES
# define GO_MAX_CHOICES 4
#endif
#ifndef GO_MAX_CASES
# define GO_MAX_CASES 3
#endif
#ifndef GO_MAX_CHANS
# define GO_MAX_CHANS 8
#endif
#ifndef GO_MAX_CAP
# define GO_MAX_CAP 4
#endif
#ifndef GO_MAX_QUEUE
# define GO_MAX_QUEUE 4
#endif
#ifndef GO_MAX_CTXS
# define GO_MAX_CTXS 4
#endif

#define go_panic(msg) assert(0 && msg)

/* ---- goroutines ---- */

static inline void go(void *(*fn)(void *), void *arg)
{
	pthread_t t;

	pthread_create(&t, NULL, fn, arg);
}

/* a goroutine blocked forever, e.g. on a nil channel or an empty select{} */
static inline void go_block_forever(void)
{
	__VERIFIER_await(0);
}

/* ---- the scheduler lock and select's random choice ---- */

static lock_t go_sched;
static atomic_int go_coin[GO_MAX_CHOICES];
static int go_next_coin;	/* under go_sched */

static void *go_coin_thread(void *arg)
{
	for (int i = 0; i < GO_MAX_CHOICES; i++)
		for (int v = 1; v < GO_MAX_CASES; v++)
			atomic_store_explicit(&go_coin[i], v, memory_order_relaxed);
	return NULL;
}

static inline void go_init(void)
{
	go(go_coin_thread, NULL);
}

/*
 * One of 0..n-1; call with go_sched held. The coin reads 0..GO_MAX_CASES-1;
 * taking it modulo n repeats some choices rather than cutting executions
 * off with an assume: under -check-liveness, GenMC reports goroutines
 * waiting for a goroutine that an assume has stopped.
 */
static inline int go_choose(int n)
{
	if (n <= 1 || go_next_coin >= GO_MAX_CHOICES)
		return 0;
	return atomic_load_explicit(&go_coin[go_next_coin++], memory_order_relaxed) % n;
}

/* ---- channels ---- */

struct go_sudog {
	long elem;		/* a received value */
	bool ok;		/* receive: false if woken by close */
	bool closed;		/* send: woken by close, so panic */
	int which;		/* the case that fired */
	atomic_bool done;
};

struct go_waiter {
	struct go_sudog *sg;
	int cas;		/* the case index, for select */
	long val;		/* a value to send */
};

typedef struct go_chan {
	int cap;
	int count, recvx, sendx;
	bool closed;
	long buf[GO_MAX_CAP];
	struct go_waiter recvq[GO_MAX_QUEUE];
	int nrecv;
	struct go_waiter sendq[GO_MAX_QUEUE];
	int nsend;
} go_chan_t;

static go_chan_t go_chans[GO_MAX_CHANS];
static atomic_int go_nchans;

/* make(chan T, cap) */
static inline go_chan_t *go_make_chan(int cap)
{
	int i = atomic_fetch_add_explicit(&go_nchans, 1, memory_order_relaxed);

	assert(i < GO_MAX_CHANS && cap <= GO_MAX_CAP);
	go_chans[i].cap = cap;
	return &go_chans[i];
}

static inline void go_enqueue(struct go_waiter *q, int *n, struct go_sudog *sg, int cas,
			      long val)
{
	assert(*n < GO_MAX_QUEUE);
	q[*n].sg = sg;
	q[*n].cas = cas;
	q[*n].val = val;
	(*n)++;
}

/* field by field: GenMC cannot always follow a struct copy (memcpy) */
static inline void go_waiter_copy(struct go_waiter *dst, const struct go_waiter *src)
{
	dst->sg = src->sg;
	dst->cas = src->cas;
	dst->val = src->val;
}

/* the oldest waiter whose select has not fired yet */
static inline bool go_dequeue(struct go_waiter *q, int *n, struct go_waiter *w)
{
	while (*n > 0) {
		go_waiter_copy(w, &q[0]);
		for (int i = 1; i < *n; i++)
			go_waiter_copy(&q[i - 1], &q[i]);
		(*n)--;
		if (!atomic_load_explicit(&w->sg->done, memory_order_relaxed))
			return true;
	}
	return false;
}

static inline bool go_has_waiter(struct go_waiter *q, int n)
{
	for (int i = 0; i < n; i++)
		if (!atomic_load_explicit(&q[i].sg->done, memory_order_relaxed))
			return true;
	return false;
}

static inline void go_remove(struct go_waiter *q, int *n, struct go_sudog *sg)
{
	int j = 0;

	for (int i = 0; i < *n; i++)
		if (q[i].sg != sg)
			go_waiter_copy(&q[j++], &q[i]);
	*n = j;
}

static inline void go_wake(struct go_waiter *w)
{
	w->sg->which = w->cas;
	atomic_store_explicit(&w->sg->done, true, memory_order_release);
}

static inline void go_park(struct go_sudog *sg)
{
	__VERIFIER_await(atomic_load_explicit(&sg->done, memory_order_acquire));
}

static inline bool go_send_ready(go_chan_t *c)
{
	return c->closed || go_has_waiter(c->recvq, c->nrecv) || c->count < c->cap;
}

static inline bool go_recv_ready(go_chan_t *c)
{
	return c->closed || go_has_waiter(c->sendq, c->nsend) || c->count > 0;
}

/* with go_sched held and go_send_ready(c) */
static inline void go_send_locked(go_chan_t *c, long v)
{
	struct go_waiter w;

	if (c->closed) {
		lock_release(&go_sched);
		go_panic("send on closed channel");
	}
	if (go_dequeue(c->recvq, &c->nrecv, &w)) {
		w.sg->elem = v;
		w.sg->ok = true;
		go_wake(&w);
		return;
	}
	c->buf[c->sendx] = v;
	c->sendx = (c->sendx + 1) % c->cap;
	c->count++;
}

/* with go_sched held and go_recv_ready(c) */
static inline void go_recv_locked(go_chan_t *c, long *v, bool *ok)
{
	struct go_waiter w;
	long val;

	if (go_dequeue(c->sendq, &c->nsend, &w)) {
		if (c->cap == 0) {
			val = w.val;
		} else {
			/* full buffer: take the head, the sender's value goes last */
			val = c->buf[c->recvx];
			c->buf[c->recvx] = w.val;
			c->recvx = (c->recvx + 1) % c->cap;
			c->sendx = c->recvx;
		}
		go_wake(&w);
		*ok = true;
	} else if (c->count > 0) {
		val = c->buf[c->recvx];
		c->recvx = (c->recvx + 1) % c->cap;
		c->count--;
		*ok = true;
	} else {
		val = 0;	/* closed and drained */
		*ok = false;
	}
	if (v)
		*v = val;
}

/* c <- v */
static inline void go_send(go_chan_t *c, long v)
{
	struct go_sudog sg = { 0 };

	if (!c)
		go_block_forever();
	lock_acquire(&go_sched);
	if (go_send_ready(c)) {
		go_send_locked(c, v);
		lock_release(&go_sched);
		return;
	}
	go_enqueue(c->sendq, &c->nsend, &sg, 0, v);
	lock_release(&go_sched);
	go_park(&sg);
	if (sg.closed)
		go_panic("send on closed channel");
}

/* v, ok := <-c; v and ok may be NULL */
static inline long go_recv2(go_chan_t *c, bool *ok)
{
	struct go_sudog sg = { 0 };
	long v;
	bool k;

	if (!c)
		go_block_forever();
	lock_acquire(&go_sched);
	if (go_recv_ready(c)) {
		go_recv_locked(c, &v, &k);
		lock_release(&go_sched);
	} else {
		go_enqueue(c->recvq, &c->nrecv, &sg, 0, 0);
		lock_release(&go_sched);
		go_park(&sg);
		v = sg.elem;
		k = sg.ok;
	}
	if (ok)
		*ok = k;
	return v;
}

static inline long go_recv(go_chan_t *c)
{
	return go_recv2(c, NULL);
}

/* close(c) */
static inline void go_close(go_chan_t *c)
{
	struct go_waiter w;

	if (!c)
		go_panic("close of nil channel");
	lock_acquire(&go_sched);
	if (c->closed) {
		lock_release(&go_sched);
		go_panic("close of closed channel");
	}
	c->closed = true;
	while (go_dequeue(c->recvq, &c->nrecv, &w)) {
		w.sg->elem = 0;
		w.sg->ok = false;
		go_wake(&w);
	}
	while (go_dequeue(c->sendq, &c->nsend, &w)) {
		w.sg->closed = true;
		go_wake(&w);
	}
	lock_release(&go_sched);
}

/* ---- select ---- */

struct go_case {
	go_chan_t *c;		/* NULL: the case never fires */
	bool send;
	long val;		/* send: the value */
	long *recv;		/* receive: where the value goes, or NULL */
	bool *ok;		/* receive: whether a value came, or NULL */
};

#define GO_SEND(ch, v)		((struct go_case){ .c = (ch), .send = true, .val = (v) })
#define GO_RECV(ch, p)		((struct go_case){ .c = (ch), .recv = (p) })
#define GO_RECV2(ch, p, k)	((struct go_case){ .c = (ch), .recv = (p), .ok = (k) })

static inline void go_case_locked(struct go_case *cs)
{
	bool k;

	if (cs->send)
		go_send_locked(cs->c, cs->val);
	else {
		go_recv_locked(cs->c, cs->recv, &k);
		if (cs->ok)
			*cs->ok = k;
	}
}

/* returns the index of the case that fired, or -1 for default */
static inline int go_select(struct go_case *cs, int n, bool has_default)
{
	struct go_sudog sg = { 0 };
	int ready[GO_MAX_CASES], nready = 0;

	assert(n <= GO_MAX_CASES);
	lock_acquire(&go_sched);
	for (int i = 0; i < n; i++)
		if (cs[i].c && (cs[i].send ? go_send_ready(cs[i].c) : go_recv_ready(cs[i].c)))
			ready[nready++] = i;
	if (nready > 0) {
		int i = ready[go_choose(nready)];
		go_case_locked(&cs[i]);
		lock_release(&go_sched);
		return i;
	}
	if (has_default) {
		lock_release(&go_sched);
		return -1;
	}
	for (int i = 0; i < n; i++) {
		if (!cs[i].c)
			continue;
		if (cs[i].send)
			go_enqueue(cs[i].c->sendq, &cs[i].c->nsend, &sg, i, cs[i].val);
		else
			go_enqueue(cs[i].c->recvq, &cs[i].c->nrecv, &sg, i, 0);
	}
	lock_release(&go_sched);
	go_park(&sg);

	lock_acquire(&go_sched);
	for (int i = 0; i < n; i++) {
		if (!cs[i].c)
			continue;
		if (cs[i].send)
			go_remove(cs[i].c->sendq, &cs[i].c->nsend, &sg);
		else
			go_remove(cs[i].c->recvq, &cs[i].c->nrecv, &sg);
	}
	lock_release(&go_sched);

	struct go_case *f = &cs[sg.which];
	if (f->send) {
		if (sg.closed)
			go_panic("send on closed channel");
	} else {
		if (f->recv)
			*f->recv = sg.elem;
		if (f->ok)
			*f->ok = sg.ok;
	}
	return sg.which;
}

/* ---- sync.Mutex ---- */

typedef lock_t go_mutex_t;

static inline void go_mutex_lock(go_mutex_t *m)
{
	lock_acquire(m);
}

static inline void go_mutex_unlock(go_mutex_t *m)
{
	if (!atomic_load_explicit(m, memory_order_relaxed))
		go_panic("sync: unlock of unlocked mutex");
	lock_release(m);
}

/* ---- sync.RWMutex (sync/rwmutex.go) ---- */

#define GO_RWMUTEX_MAX_READERS (1 << 30)

typedef struct {
	go_mutex_t w;
	sema_t writer_sem;
	sema_t reader_sem;
	atomic_int reader_count;	/* readers in or waiting; - MAX while a writer waits */
	atomic_int reader_wait;		/* readers the waiting writer still waits for */
} go_rwmutex_t;

static inline void go_rwmutex_rlock(go_rwmutex_t *rw)
{
	if (atomic_fetch_add(&rw->reader_count, 1) + 1 < 0)
		sema_wait(&rw->reader_sem);	/* a writer is pending */
}

static inline void go_rwmutex_runlock(go_rwmutex_t *rw)
{
	int r = atomic_fetch_sub(&rw->reader_count, 1) - 1;

	if (r < 0) {
		if (r + 1 == 0 || r + 1 == -GO_RWMUTEX_MAX_READERS)
			go_panic("sync: RUnlock of unlocked RWMutex");
		/* the last reader out lets the pending writer in */
		if (atomic_fetch_sub(&rw->reader_wait, 1) - 1 == 0)
			sema_signal(&rw->writer_sem);
	}
}

static inline void go_rwmutex_lock(go_rwmutex_t *rw)
{
	go_mutex_lock(&rw->w);
	int r = atomic_fetch_sub(&rw->reader_count, GO_RWMUTEX_MAX_READERS);
	if (r != 0 && atomic_fetch_add(&rw->reader_wait, r) + r != 0)
		sema_wait(&rw->writer_sem);
}

static inline void go_rwmutex_unlock(go_rwmutex_t *rw)
{
	int r = atomic_fetch_add(&rw->reader_count, GO_RWMUTEX_MAX_READERS) +
		GO_RWMUTEX_MAX_READERS;

	if (r >= GO_RWMUTEX_MAX_READERS)
		go_panic("sync: Unlock of unlocked RWMutex");
	for (int i = 0; i < r; i++)
		sema_signal(&rw->reader_sem);
	go_mutex_unlock(&rw->w);
}

/* ---- sync.WaitGroup ---- */

typedef struct {
	atomic_int v;
} go_waitgroup_t;

static inline void go_wg_add(go_waitgroup_t *wg, int n)
{
	if (atomic_fetch_add(&wg->v, n) + n < 0)
		go_panic("sync: negative WaitGroup counter");
}

static inline void go_wg_done(go_waitgroup_t *wg)
{
	go_wg_add(wg, -1);
}

static inline void go_wg_wait(go_waitgroup_t *wg)
{
	__VERIFIER_await(atomic_load(&wg->v) == 0);
}

/* ---- sync.Cond, with a sync.Mutex as its Locker ---- */

typedef cond_t go_cond_t;

static inline void go_cond_wait(go_cond_t *c, go_mutex_t *m)
{
	cond_wait(c, m);
}

static inline void go_cond_signal(go_cond_t *c)
{
	cond_signal(c);
}

static inline void go_cond_broadcast(go_cond_t *c)
{
	cond_broadcast(c);
}

/* ---- sync.Once ---- */

typedef struct {
	atomic_int done;
	go_mutex_t m;
} go_once_t;

static inline void go_once_do(go_once_t *o, void (*fn)(void *), void *arg)
{
	if (atomic_load(&o->done))
		return;
	go_mutex_lock(&o->m);
	if (!atomic_load_explicit(&o->done, memory_order_relaxed)) {
		fn(arg);
		atomic_store(&o->done, 1);
	}
	go_mutex_unlock(&o->m);
}

/* ---- timers: time.After, time.NewTimer ---- */

static void *go_timer_fire(void *arg)
{
	go_send((go_chan_t *) arg, 1);
	return NULL;
}

/* a channel that receives one value at an arbitrary point */
static inline go_chan_t *go_time_after(void)
{
	go_chan_t *c = go_make_chan(1);

	go(go_timer_fire, c);
	return c;
}

/* ---- context: cancellation only ---- */

typedef struct go_ctx {
	go_chan_t *done;		/* NULL for Background(): never done */
	bool cancelled;			/* under go_sched */
	struct go_ctx *children[GO_MAX_CTXS];
	int nchildren;			/* under go_sched */
} go_ctx_t;

static go_ctx_t go_ctxs[GO_MAX_CTXS];
static atomic_int go_nctxs;
static go_ctx_t go_ctx_background_ctx;

static inline go_ctx_t *go_ctx_background(void)
{
	return &go_ctx_background_ctx;
}

static inline go_chan_t *go_ctx_done(go_ctx_t *ctx)
{
	return ctx->done;
}

static inline void go_ctx_cancel(go_ctx_t *ctx)
{
	lock_acquire(&go_sched);
	if (ctx->cancelled) {
		lock_release(&go_sched);
		return;
	}
	ctx->cancelled = true;
	int n = ctx->nchildren;
	lock_release(&go_sched);
	go_close(ctx->done);
	for (int i = 0; i < n; i++)
		go_ctx_cancel(ctx->children[i]);
}

/* ctx, cancel := context.WithCancel(parent); cancel is go_ctx_cancel(ctx) */
static inline go_ctx_t *go_ctx_with_cancel(go_ctx_t *parent)
{
	int i = atomic_fetch_add_explicit(&go_nctxs, 1, memory_order_relaxed);
	go_ctx_t *ctx = &go_ctxs[i];
	bool cancel_now = false;

	assert(i < GO_MAX_CTXS);
	ctx->done = go_make_chan(0);
	if (parent->done) {
		lock_acquire(&go_sched);
		if (parent->cancelled) {
			cancel_now = true;
		} else {
			assert(parent->nchildren < GO_MAX_CTXS);
			parent->children[parent->nchildren++] = ctx;
		}
		lock_release(&go_sched);
	}
	if (cancel_now)
		go_ctx_cancel(ctx);
	return ctx;
}

static void *go_ctx_timer(void *arg)
{
	go_ctx_cancel((go_ctx_t *) arg);
	return NULL;
}

/* context.WithTimeout(parent, d): cancelled at an arbitrary point */
static inline go_ctx_t *go_ctx_with_timeout(go_ctx_t *parent)
{
	go_ctx_t *ctx = go_ctx_with_cancel(parent);

	go(go_ctx_timer, ctx);
	return ctx;
}

#endif /* GOBENCH_GO_H */
