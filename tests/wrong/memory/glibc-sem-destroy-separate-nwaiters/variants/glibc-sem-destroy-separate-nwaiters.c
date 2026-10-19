/* glibc sem_post reads nwaiters after adding the token: the waiter has freed the semaphore (bug 12674) */
#define BUG_SEPARATE_NWAITERS
#include "../../../../correct/classic/glibc-sem-destroy/glibc-sem-destroy.c"
