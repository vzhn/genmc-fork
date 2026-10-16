/* waitqueue_active() without smp_mb(): the waiter sleeps forever */
#define BUG_NO_MB
#include "../../../../correct/classic/waitqueue/waitqueue.c"
