#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <assert.h>
#include <lkmm.h>

/*
 * Removing an entry from a list that RCU readers traverse (Linux
 * include/linux/rculist.h), written with the kernel's primitives
 * (<lkmm.h>); tests/correct/classic/rcu-list-del is the same model in C11
 * atomics, without the grace period. list_del_rcu() unlinks the entry with
 * a WRITE_ONCE() into its predecessor and leaves the entry's own next
 * pointer intact, so a reader that is standing on the entry can still move
 * on; the entry is freed after synchronize_rcu(). list_del() instead
 * poisons the entry's pointers (LIST_POISON1), which is fine for lists that
 * readers access under a lock only.
 *
 * The list is A -> B -> C. The reader walks it in a read-side critical
 * section (rcu_dereference()) and sums the values; the writer removes B,
 * waits for a grace period and frees it. The reader sees 1 + 3 or
 * 1 + 2 + 3.
 *
 * BUG_LIST_DEL: the writer uses list_del() (list.h), which also sets B's
 * next pointer to the poison value. A reader that already reached B follows
 * the poison: the real kernel faults, GenMC reports an access to memory
 * that was never allocated. The fix in such cases is list_del_rcu().
 *
 * BUG_NO_GP: the writer frees B right after unlinking it, without
 * synchronize_rcu(). A reader standing on B reads freed memory.
 */

#define LIST_POISON1 ((struct entry *) 0x100)

struct entry {
	int value;
	struct entry *next;
};

struct entry *head;
int sum;

static void list_del_entry(struct entry **prev_next, struct entry *e)
{
	/* __list_del() reads e->next plainly; READ_ONCE() sidesteps a false
	 * race GenMC reports under -imm when a revisit replays this thread */
	WRITE_ONCE(*prev_next, READ_ONCE(e->next));
#ifdef BUG_LIST_DEL
	WRITE_ONCE(e->next, LIST_POISON1);	/* list_del(): marked, so the error is the dereference */
#endif
}

static void *reader(void *unused)
{
	rcu_read_lock();
	for (struct entry *e = rcu_dereference(head); e; e = rcu_dereference(e->next))
		sum += e->value;
	rcu_read_unlock();
	return NULL;
}

static void *writer(void *unused)
{
	struct entry *a = head, *b = a->next;

	list_del_entry(&a->next, b);
#ifndef BUG_NO_GP
	synchronize_rcu();
#endif
	free(b);
	return NULL;
}

int main()
{
	pthread_t r, w;
	struct entry *a = malloc(sizeof(*a)), *b = malloc(sizeof(*b)), *c = malloc(sizeof(*c));

	a->value = 1;
	b->value = 2;
	c->value = 3;
	c->next = NULL;
	b->next = c;
	a->next = b;
	head = a;

	if (pthread_create(&r, NULL, reader, NULL))
		abort();
	if (pthread_create(&w, NULL, writer, NULL))
		abort();
	if (pthread_join(r, NULL))
		abort();
	if (pthread_join(w, NULL))
		abort();
	assert(sum == 4 || sum == 6);
	free(c);
	free(a);
	return 0;
}
