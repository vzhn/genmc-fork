/* rcu_dereference() paired with WRITE_ONCE() instead of rcu_assign_pointer(): the reader sees the object uninitialised */
#define BUG_WRITE_ONCE
#include "../../../../correct/lkmm/rcu-publish/rcu-publish.c"
