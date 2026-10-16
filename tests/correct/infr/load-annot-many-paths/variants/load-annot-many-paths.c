#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <genmc.h>

// 40 one-sided branches between the load and the assume: 2^40 CFG paths.
// Annotating the load must not enumerate them.
atomic_ullong x;
int g[40];

void *writer(void *arg)
{
	atomic_store_explicit(&x, 1, memory_order_release);
	return NULL;
}

int main()
{
	pthread_t t;

	pthread_create(&t, NULL, writer, NULL);
	unsigned long long v = atomic_load_explicit(&x, memory_order_acquire);
	if (v & (1ULL << 0))
		g[0] = 1;
	if (v & (1ULL << 1))
		g[1] = 1;
	if (v & (1ULL << 2))
		g[2] = 1;
	if (v & (1ULL << 3))
		g[3] = 1;
	if (v & (1ULL << 4))
		g[4] = 1;
	if (v & (1ULL << 5))
		g[5] = 1;
	if (v & (1ULL << 6))
		g[6] = 1;
	if (v & (1ULL << 7))
		g[7] = 1;
	if (v & (1ULL << 8))
		g[8] = 1;
	if (v & (1ULL << 9))
		g[9] = 1;
	if (v & (1ULL << 10))
		g[10] = 1;
	if (v & (1ULL << 11))
		g[11] = 1;
	if (v & (1ULL << 12))
		g[12] = 1;
	if (v & (1ULL << 13))
		g[13] = 1;
	if (v & (1ULL << 14))
		g[14] = 1;
	if (v & (1ULL << 15))
		g[15] = 1;
	if (v & (1ULL << 16))
		g[16] = 1;
	if (v & (1ULL << 17))
		g[17] = 1;
	if (v & (1ULL << 18))
		g[18] = 1;
	if (v & (1ULL << 19))
		g[19] = 1;
	if (v & (1ULL << 20))
		g[20] = 1;
	if (v & (1ULL << 21))
		g[21] = 1;
	if (v & (1ULL << 22))
		g[22] = 1;
	if (v & (1ULL << 23))
		g[23] = 1;
	if (v & (1ULL << 24))
		g[24] = 1;
	if (v & (1ULL << 25))
		g[25] = 1;
	if (v & (1ULL << 26))
		g[26] = 1;
	if (v & (1ULL << 27))
		g[27] = 1;
	if (v & (1ULL << 28))
		g[28] = 1;
	if (v & (1ULL << 29))
		g[29] = 1;
	if (v & (1ULL << 30))
		g[30] = 1;
	if (v & (1ULL << 31))
		g[31] = 1;
	if (v & (1ULL << 32))
		g[32] = 1;
	if (v & (1ULL << 33))
		g[33] = 1;
	if (v & (1ULL << 34))
		g[34] = 1;
	if (v & (1ULL << 35))
		g[35] = 1;
	if (v & (1ULL << 36))
		g[36] = 1;
	if (v & (1ULL << 37))
		g[37] = 1;
	if (v & (1ULL << 38))
		g[38] = 1;
	if (v & (1ULL << 39))
		g[39] = 1;
	__VERIFIER_assume(v == 1);
	assert(g[0] == 1);
	pthread_join(t, NULL);
	return 0;
}
