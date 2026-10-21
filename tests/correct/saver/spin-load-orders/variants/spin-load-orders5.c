/* A spin loop whose check before the loop reads with a different order
 * (acquire) than the loop (seq_cst): the two reads stay apart, and the loop is
 * still a spin loop (#91: it was not, and GenMC did not terminate) */

#include <pthread.h>
#include <stdatomic.h>

atomic_int flag = 1;

void *waiter(void *arg)
{
	int w = atomic_load_explicit(&flag, memory_order_acquire);
	while (w)
		w = atomic_load_explicit(&flag, memory_order_seq_cst);
	return NULL;
}

void *releaser(void *arg)
{
	atomic_store_explicit(&flag, 0, memory_order_release);
	return NULL;
}

int main()
{
	pthread_t a, b;
	pthread_create(&a, NULL, waiter, NULL);
	pthread_create(&b, NULL, releaser, NULL);
	pthread_join(a, NULL);
	pthread_join(b, NULL);
	return 0;
}
