#include "../../../lib/go.h"

/*
 * GoBench kernel cockroach#10214 (blocking, AB-BA deadlock), after
 * CockroachDB's storage.Store heartbeat coalescing. Two locks: the store's
 * coalescedMu (A) and each replica's raftMu (B).
 *
 * - sendQueuedHeartbeats() takes A and, for each queued heartbeat response,
 *   reports every replica unreachable, which takes that replica's B.
 * - A replica's tick() takes its B (and its mu), and then coalesces its
 *   heartbeat, which takes A.
 *
 * One goroutine sends the queued heartbeats while another ticks a replica.
 *
 * BUG_SEND_HOLDS_COALESCED_MU: sendQueuedHeartbeats() holds A while it takes
 * B, as before CockroachDB commit 27e863d90ab0 (2016), which takes the
 * queued responses out under A and releases A before sending them.
 */

#define NREPLICAS 2
#define NRESPONSES 2

struct replica {
	go_mutex_t raft_mu;	/* B */
	go_mutex_t mu;
};

go_mutex_t coalesced_mu;	/* A */
int heartbeat_responses = NRESPONSES;
struct replica replicas[NREPLICAS];

static void report_unreachable(struct replica *r)
{
	go_mutex_lock(&r->raft_mu);
	go_mutex_unlock(&r->raft_mu);
}

static void send_queued_heartbeats_to_node(void)
{
	for (int i = 0; i < NREPLICAS; i++)
		report_unreachable(&replicas[i]);
}

void *send_queued_heartbeats(void *arg)
{
	go_mutex_lock(&coalesced_mu);
	int n = heartbeat_responses;
#ifdef BUG_SEND_HOLDS_COALESCED_MU
	for (int i = 0; i < n; i++)
		send_queued_heartbeats_to_node();
	go_mutex_unlock(&coalesced_mu);
#else
	heartbeat_responses = 0;
	go_mutex_unlock(&coalesced_mu);
	for (int i = 0; i < n; i++)
		send_queued_heartbeats_to_node();
#endif
	return NULL;
}

/* rp1.tick() */
void *tick(void *arg)
{
	struct replica *r = &replicas[0];

	go_mutex_lock(&r->raft_mu);
	go_mutex_lock(&r->mu);
	/* maybeQuiesceLocked() -> maybeCoalesceHeartbeat() */
	go_mutex_lock(&coalesced_mu);
	go_mutex_unlock(&coalesced_mu);
	go_mutex_unlock(&r->mu);
	go_mutex_unlock(&r->raft_mu);
	return NULL;
}

int main()
{
	go_init();
	go(send_queued_heartbeats, NULL);
	go(tick, NULL);
	return 0;
}
