/* 2 executions and no "Unordered writes" warning: the writes are
 * ordered by the mutex (issue #98, scripts/check-lkmm-ww-warning.sh) */
#include <pthread.h>
#include <lkmm.h>
static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
static int data;
static void *t(void *a) { pthread_mutex_lock(&m); data = (int)(long)a; pthread_mutex_unlock(&m); return NULL; }
int main() { pthread_t a, b; pthread_create(&a, NULL, t, (void *)1); pthread_create(&b, NULL, t, (void *)2); pthread_join(a, NULL); pthread_join(b, NULL); return 0; }
