/* waitqueue_active() without smp_mb(): the waiter sleeps forever (kernel primitives) */
#define BUG_NO_MB
#include "../../../../correct/lkmm/waitqueue/waitqueue.c"
