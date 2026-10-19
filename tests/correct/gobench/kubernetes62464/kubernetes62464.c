#include "../../../lib/go.h"

/*
 * GoBench kernel kubernetes#62464 (blocking, recursive read lock), after the
 * kubelet's CPU manager state (pkg/kubelet/cm/cpumanager/state). Its getters
 * take an RWMutex for reading; GetCPUSetOrDefault() took it and then called
 * the other getters, which take it again. A setter takes it for writing.
 *
 * If the setter's Lock() comes between the two RLock()s, the reader waits
 * for the writer, which waits for the reader.
 *
 * BUG_RECURSIVE_RLOCK: GetCPUSetOrDefault() holds the read lock around its
 * calls, as before Kubernetes commit c1b19fce9036 ("avoid double RLock() in
 * cpumanager", 2018).
 */

go_rwmutex_t s;

static bool get_cpu_set(void)
{
	go_rwmutex_rlock(&s);
	go_rwmutex_runlock(&s);
	return false;		/* the container has no assignment */
}

static void get_default_cpu_set(void)
{
	go_rwmutex_rlock(&s);
	go_rwmutex_runlock(&s);
}

static void set_default_cpu_set(void)
{
	go_rwmutex_lock(&s);
	go_rwmutex_unlock(&s);
}

static void get_cpu_set_or_default(void)
{
#ifdef BUG_RECURSIVE_RLOCK
	go_rwmutex_rlock(&s);
#endif
	if (!get_cpu_set())
		get_default_cpu_set();
#ifdef BUG_RECURSIVE_RLOCK
	go_rwmutex_runlock(&s);
#endif
}

/* G1: m.reconcileState() */
void *reconcile_state(void *arg)
{
	get_cpu_set_or_default();
	return NULL;
}

/* G2: p.RemoveContainer() */
void *remove_container(void *arg)
{
	get_default_cpu_set();
	set_default_cpu_set();
	return NULL;
}

int main()
{
	go_init();
	go(reconcile_state, NULL);
	go(remove_container, NULL);
	return 0;
}
