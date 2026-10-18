/* kfree() right after list_del_rcu(), without synchronize_rcu(): a reader uses freed memory */
#define BUG_NO_GP
#include "../../../../correct/lkmm/rcu-list-del/rcu-list-del.c"
