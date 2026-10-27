/*
 * IMM orders the read of x before the write of z: the read of y[a] depends
 * on it by address, and IMM's deps have addr ; po?. With thread_2's fence
 * between its writes of x1 and x, thread_0 cannot read x == 1 while
 * thread_1 reads z == 1 and then, after its fence, x1 == 0 (an ar cycle:
 * x -rfe-> R(x) -ppo-> W(z) -rfe-> R(z) -FSC;hb;eco;hb;FSC-> F -ppo-> W(x)).
 * GenMC explores IMM without the po? (a revisit can delete the read of y[a]
 * and keep the write of z, a hole no way of filling keeps consistent,
 * vzhn/kater-fork#38), and checks IMM's own ar of complete executions and
 * of the graph of an error: the assertion must not fail.
 */

atomic_int x;
atomic_int y[2];
atomic_int z;
atomic_int x1;

int a, r, s;

void *thread_0(void *unused)
{
	a = atomic_load_explicit(&x, memory_order_relaxed);
	(void)atomic_load_explicit(&y[a], memory_order_relaxed);
	atomic_store_explicit(&z, 1, memory_order_relaxed);
	return NULL;
}

void *thread_1(void *unused)
{
	r = atomic_load_explicit(&z, memory_order_relaxed);
	atomic_thread_fence(memory_order_seq_cst);
	s = atomic_load_explicit(&x1, memory_order_relaxed);
	return NULL;
}

void *thread_2(void *unused)
{
	atomic_store_explicit(&x1, 1, memory_order_relaxed);
	atomic_thread_fence(memory_order_seq_cst);
	atomic_store_explicit(&x, 1, memory_order_relaxed);
	return NULL;
}
