/* parking_lot before 3fe7233ee05f: a relaxed CAS in wait_for_readers() */
#define BUG_RELAXED_PARK_CAS
#include "../../../../correct/classic/parking-lot-rwlock/parking-lot-rwlock.c"
