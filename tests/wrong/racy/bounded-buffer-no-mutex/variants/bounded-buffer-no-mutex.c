/* The buffer is not guarded: two producers write the same slot */
#define BUG_NO_MUTEX
#define CAP 2
#include "../../../../correct/classic/bounded-buffer/bounded-buffer.c"
