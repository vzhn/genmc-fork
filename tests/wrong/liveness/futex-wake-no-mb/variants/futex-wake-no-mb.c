/* futex_wake() without barrier (B), fixed by Linux 76835b0ebf8a: lost wakeup */
#define BUG_NO_MB
#include "../../../../correct/classic/futex-wake/futex-wake.c"
