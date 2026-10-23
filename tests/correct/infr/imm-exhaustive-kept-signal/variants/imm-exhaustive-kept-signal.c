/* -exhaustive under -imm: a revisit keeps some events without their prefix
 * (see keepValueDependencies()), here a producer's pthread_cond_signal()
 * write that a kept waiter reads. The copy of the graph kept the cached
 * prefix view of that write, computed in the old graph, which names events
 * the copy does not have: -check-invariants stopped the exploration (I1),
 * although backwardRevisit() drops these views before any use. They now go
 * with the copy (#181). The revisit view could also end a thread at one of
 * the graph's holes, which the copy then ended with (I1).
 *
 * (-disable-race-detection: the false race of #179.)
 */
#include "../../pthread-cond/variants/pthread-cond.c"
