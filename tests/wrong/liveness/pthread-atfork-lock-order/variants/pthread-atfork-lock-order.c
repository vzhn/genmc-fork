/*
 * The prepare handlers take b, then a (the reverse of registration order),
 * while a worker takes a, then b: the fork can deadlock with the worker.
 */
#define BUG_LOCK_ORDER
#include "../../../../correct/infr/pthread-atfork/variants/pthread-atfork.c"
