/* -exhaustive under -imm: a revisit kept a producer's pthread_cond_signal()
 * (a kept waiter reads it) without the producer's items++ before it, a
 * hole. The producer then fills the hole: its items++ is hb-before the
 * waiter's kept read of items (po, then the signal, read with acquire),
 * but the race check at the items++ took the waiter's read for one added
 * before it, and reported a false "Non-atomic race!" (#179). Once that
 * was ordered both ways, a race with a kept read the consumer had not
 * reached yet came next: its thread re-runs a CAS that now fails, and the
 * read is dropped. A race is now checked, at an event, only against
 * accesses not after a hole, and the execution is checked as a whole once
 * its threads have reached their kept events.
 */
#include "../../pthread-cond/variants/pthread-cond.c"
