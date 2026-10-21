/*
 * An init routine that calls pthread_once() on its own control waits for
 * itself to finish.
 */
#define BUG_ONCE_RECURSIVE
#include "../../../../correct/infr/pthread-once-key/variants/pthread-once-key.c"
