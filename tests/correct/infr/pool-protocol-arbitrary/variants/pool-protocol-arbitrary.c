/*
 * The fixed pool protocol under arbitrary schedules (args.*.mo.in fix the
 * seeds; without -check-liveness, hence fewer executions than in
 * pool-protocol). Under IMM, a backward revisit copies the graph up to a
 * view with holes; a thread whose create event became a hole kept its
 * begin label, and re-executing the create then found the "already
 * existing" thread (INTERNAL FAILURE in handleThreadCreate). The schedule
 * drawn from a seed depends on the compiled code, so the seeds only
 * reproduce the failure with some LLVM versions; nested-create-revisit
 * covers the same code path independently of the schedule.
 */
#define FIXED
#include "../../pool-protocol/pool-model.c"
