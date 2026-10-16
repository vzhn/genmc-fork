#include <assert.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>

static int count(const uint8_t *p, int n)
{
	int c = 0;
	for (int i = 0; i < n; i++)
		c += (!p || p[i] == 0) ? 1 : 0;
	return c;
}

uint8_t arr[3] = { 0, 1, 0 };
_Atomic(uint8_t *) gp;

int main()
{
	atomic_store_explicit(&gp, arr, memory_order_relaxed);
	assert(count(atomic_load_explicit(&gp, memory_order_relaxed), 3) == 2);
	assert(count(NULL, 3) == 3);
	return 0;
}
