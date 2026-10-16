/* Two llist_del_first() without a lock: ABA loses a node (llist.h) */
#define BUG_NO_LOCK
#include "../../../../correct/data-structures/llist/llist.c"
