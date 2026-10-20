/* 2 executions and an "Unordered writes" warning: nothing orders the
 * writes (scripts/check-lkmm-ww-warning.sh) */
#include <pthread.h>
#include <lkmm.h>
int x;
void *t(void *a) { WRITE_ONCE(x, (int)(long)a); return NULL; }
int main() { pthread_t a, b; pthread_create(&a, NULL, t, (void *)1); pthread_create(&b, NULL, t, (void *)2); pthread_join(a, NULL); pthread_join(b, NULL); return 0; }
