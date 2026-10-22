/* -exhaustive under -imm: a revisit keeps, with each kept event, the
 * creation of its thread (see keepValueDependencies()), and every view
 * contains the begin of every thread, so a thread that had only its begin
 * kept its create too. Here t2's store revisits t1's load: the graph kept
 * main's creation of t3, but not the join of t1 before it, a hole. The
 * views of the kept create (and so of t3) were computed without the join,
 * and stayed so once the hole was filled: t3's write raced with t1's (a
 * false "Non-atomic race!"; the default exploration finds none). The
 * begin of t3, kept with the create, also kept its cached prefix view,
 * which names t1's end, an event the copy does not have
 * (-check-invariants: I1). A begin keeps nothing: t3 starts anew after
 * the join.
 *
 * (Reduced from tests/correct/saver/spin-fmutex.)
 */
#include <pthread.h>
#include <stdatomic.h>

int shared;
atomic_int y;
atomic_int z;

void *t1(void *arg)
{
	(void) atomic_load_explicit(&y, memory_order_relaxed);
	shared = 1;
	return NULL;
}

void *t2(void *arg)
{
	(void) atomic_load_explicit(&z, memory_order_relaxed);
	atomic_store_explicit(&y, 1, memory_order_relaxed);
	return NULL;
}

void *t3(void *arg)
{
	shared = 3;
	return NULL;
}

int main(void)
{
	pthread_t h1, h2, h3;
	pthread_create(&h1, NULL, t1, NULL);
	pthread_create(&h2, NULL, t2, NULL);
	pthread_join(h1, NULL);
	pthread_create(&h3, NULL, t3, NULL);
	pthread_join(h2, NULL);
	pthread_join(h3, NULL);
	return 0;
}
