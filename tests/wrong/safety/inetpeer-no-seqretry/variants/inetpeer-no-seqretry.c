/* inet_getpeer() returns a lockless hit without read_seqretry(): a peer with a stale refcount */
#define BUG_NO_SEQRETRY
#include "../../../../correct/classic/inetpeer-seqretry/inetpeer-seqretry.c"
