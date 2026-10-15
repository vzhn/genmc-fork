#include <assert.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>

// JumpThreading unfolds the select of "(p && p[i]) ? 0 : 1" into branches and
// freezes its condition: %cond.fr = freeze i1 %tobool
static int cost(const uint8_t *p, int n)
{
	int c = 0;

	for (int i = 0; i < n; i++)
		c += (p && p[i]) ? 0 : 1;
	return c;
}

uint8_t arr[2] = { 0, 1 };
_Atomic(uint8_t *) gp;

int main()
{
	atomic_store_explicit(&gp, arr, memory_order_relaxed);
	/* not a constant after inlining, so the select survives until JumpThreading */
	assert(cost(atomic_load_explicit(&gp, memory_order_relaxed), 2) == 1);
	return 0;
}
