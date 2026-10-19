/* crossbeam before bf1fb5627d67: the blocked consumer's ready flag is relaxed */
#define BUG_RELAXED_READY
#include "../../../../correct/classic/crossbeam-ms-queue/crossbeam-ms-queue.c"
