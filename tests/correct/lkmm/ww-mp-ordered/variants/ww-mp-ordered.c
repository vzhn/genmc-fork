/* 2 executions and no "Unordered writes" warning: the second write
 * follows an acquire of the first's release (issue #98,
 * scripts/check-lkmm-ww-warning.sh) */
#include <pthread.h>
#include <lkmm.h>
static int flag, data;
static void *t1(void *a) { data = 1; smp_store_release(&flag, 1); return NULL; }
static void *t2(void *a) { if (smp_load_acquire(&flag)) data = 2; return NULL; }
int main() { pthread_t a, b; pthread_create(&a, NULL, t1, NULL); pthread_create(&b, NULL, t2, NULL); pthread_join(a, NULL); pthread_join(b, NULL); return 0; }
