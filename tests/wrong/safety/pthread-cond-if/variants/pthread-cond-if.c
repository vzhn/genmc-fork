/*
 * A consumer that checks for an item with `if` rather than `while`: a
 * wakeup does not mean an item is there (another consumer may have taken
 * it, or the wakeup was spurious), so the assertion fails.
 */
#define BUG_IF
#include "../../../../correct/infr/pthread-cond/variants/pthread-cond.c"
