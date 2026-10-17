#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>
#include "../../../lib/lock.h"
#include "../../../lib/sem.h"

/*
 * Go's sync.RWMutex (src/sync/rwmutex.go). readerCount counts readers; a
 * writer takes the writer mutex w, subtracts rwmutexMaxReaders from
 * readerCount (so arriving readers see it negative and wait on readerSem)
 * and waits on writerSem until the readers that were inside have left
 * (readerWait). Unlock adds rwmutexMaxReaders back, releases the waiting
 * readers and unlocks w. Go's atomics are seq_cst; the runtime semaphores
 * are sema_t.
 *
 * Reader R1 read-locks, reads and unlocks; the writer W write-locks, writes
 * and unlocks; reader R2 does what R1 does. No reader runs with the writer,
 * and nobody waits forever (-check-liveness).
 *
 * BUG_RECURSIVE_RLOCK: R1 read-locks twice before unlocking, which the
 * RWMutex documentation prohibits: "If any goroutine calls Lock while the
 * lock is already held by one or more readers, concurrent calls to RLock
 * will block until the writer has acquired (and released) the lock ... In
 * particular, this prohibits recursive read-locking." When W's Lock comes
 * between R1's two RLocks, W waits for R1 and R1 waits for W: deadlock.
 */

#define MAX_READERS (1 << 30)

lock_t w;
sema_t writer_sem;
sema_t reader_sem;
atomic_int reader_count;
atomic_int reader_wait;

atomic_int readers_in, writers_in;

static void rlock(void)
{
	if (atomic_fetch_add(&reader_count, 1) + 1 < 0)
		sema_wait(&reader_sem);	/* a writer is pending */
}

static void runlock(void)
{
	int r = atomic_fetch_add(&reader_count, -1) - 1;

	/* a writer is pending: the last departing reader wakes it */
	if (r < 0 && atomic_fetch_add(&reader_wait, -1) - 1 == 0)
		sema_signal(&writer_sem);
}

static void wlock(void)
{
	lock_acquire(&w);

	/* announce the writer; r is the number of readers inside */
	int r = atomic_fetch_add(&reader_count, -MAX_READERS);

	/* wait for the active readers */
	if (r != 0 && atomic_fetch_add(&reader_wait, r) + r != 0)
		sema_wait(&writer_sem);
}

static void wunlock(void)
{
	int r = atomic_fetch_add(&reader_count, MAX_READERS) + MAX_READERS;

	for (int i = 0; i < r; i++)
		sema_signal(&reader_sem);
	lock_release(&w);
}

static void read_section(void)
{
	atomic_fetch_add(&readers_in, 1);
	assert(atomic_load(&writers_in) == 0);
	atomic_fetch_sub(&readers_in, 1);
}

static void *reader(void *arg)
{
	rlock();
#ifdef BUG_RECURSIVE_RLOCK
	if (arg)
		rlock();
#endif
	read_section();
#ifdef BUG_RECURSIVE_RLOCK
	if (arg)
		runlock();
#endif
	runlock();
	return NULL;
}

static void *writer(void *unused)
{
	wlock();
	atomic_fetch_add(&writers_in, 1);
	assert(atomic_load(&readers_in) == 0);
	atomic_fetch_sub(&writers_in, 1);
	wunlock();
	return NULL;
}

int main()
{
	pthread_t r1, r2, wr;

	if (pthread_create(&r1, NULL, reader, (void *) 1))
		abort();
	if (pthread_create(&wr, NULL, writer, NULL))
		abort();
	if (pthread_create(&r2, NULL, reader, NULL))
		abort();
	if (pthread_join(r1, NULL))
		abort();
	if (pthread_join(wr, NULL))
		abort();
	if (pthread_join(r2, NULL))
		abort();
	return 0;
}
