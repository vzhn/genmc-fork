/* seL4 CLH lock enqueues with an acquire-only exchange: two threads in the critical section */
#define BUG_EXCHANGE_ACQUIRE
#include "../../../../correct/classic/sel4-clh-lock/sel4-clh-lock.c"
