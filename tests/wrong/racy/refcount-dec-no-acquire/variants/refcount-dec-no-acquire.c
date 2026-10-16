/* refcount_dec_and_test() before Linux 47b8f3ab9c49: no acquire */
#define BUG_NO_ACQUIRE
#include "../../../../correct/classic/refcount/refcount.c"
