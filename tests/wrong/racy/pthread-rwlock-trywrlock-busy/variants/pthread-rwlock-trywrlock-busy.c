/* A writer that writes even when pthread_rwlock_trywrlock() failed races with a reader */
#define BUG_WRITE_IF_BUSY
#include "../../../../correct/infr/pthread-rwlock-trywrlock/variants/pthread-rwlock-trywrlock.c"
