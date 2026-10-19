/* glibc trywrlock overwrites __wrphase_futex in a write phase: a sleeping reader is never woken (bug 23844) */
#define BUG_TRYWRLOCK_STORE
#include "../../../../correct/classic/glibc-rwlock-trywrlock/glibc-rwlock-trywrlock.c"
