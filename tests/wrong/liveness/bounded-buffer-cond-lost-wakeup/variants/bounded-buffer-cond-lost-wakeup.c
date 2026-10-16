/* put() signals only when the buffer was empty: a consumer is never woken */
#define BUG_LOST_WAKEUP
#include "../../../../correct/classic/bounded-buffer-cond/bounded-buffer-cond.c"
