#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>

/*
 * Removing an entry from a list that RCU readers traverse (Linux
 * include/linux/rculist.h). list_del_rcu() unlinks the entry with a
 * release store into its predecessor and leaves the entry's own next
 * pointer intact, so a reader that is standing on the entry can still move
 * on; the entry may be freed only after a grace period. list_del() instead
 * poisons the entry's pointers (LIST_POISON1), which is fine for lists that
 * readers access under a lock only.
 *
 * The list is A -> B -> C. The reader walks it (rcu_dereference() is an
 * acquire load) and sums the values; the writer removes B. The reader sees
 * 1 + 3 or 1 + 2 + 3. Nothing is freed while the reader runs (main frees
 * the entries afterwards, as after a grace period).
 *
 * BUG_LIST_DEL: the writer uses list_del() (list.h), which also sets B's
 * next pointer to the poison value. A reader that already reached B follows
 * the poison: the real kernel faults, GenMC reports an access to memory
 * that was never allocated. The fix in such cases is list_del_rcu().
 */

#define LIST_POISON1 ((struct entry *) 0x100)

struct entry {
	int value;
	_Atomic(struct entry *) next;
};

struct entry a, b, c;
_Atomic(struct entry *) head;
int sum;

static void list_del_entry(_Atomic(struct entry *) *prev_next, struct entry *e)
{
	/* rcu_assign_pointer(prev->next, e->next) */
	atomic_store_explicit(prev_next, atomic_load_explicit(&e->next, memory_order_relaxed),
			      memory_order_release);
#ifdef BUG_LIST_DEL
	atomic_store_explicit(&e->next, LIST_POISON1, memory_order_relaxed);
#endif
}

static void *reader(void *unused)
{
	/* rcu_read_lock(); list_for_each_entry_rcu() */
	for (struct entry *e = atomic_load_explicit(&head, memory_order_acquire); e;
	     e = atomic_load_explicit(&e->next, memory_order_acquire))
		sum += e->value;
	/* rcu_read_unlock(); */
	return NULL;
}

static void *writer(void *unused)
{
	list_del_entry(&a.next, &b);
	return NULL;
}

int main()
{
	pthread_t r, w;

	a.value = 1;
	b.value = 2;
	c.value = 3;
	atomic_init(&c.next, NULL);
	atomic_init(&b.next, &c);
	atomic_init(&a.next, &b);
	atomic_init(&head, &a);

	if (pthread_create(&r, NULL, reader, NULL))
		abort();
	if (pthread_create(&w, NULL, writer, NULL))
		abort();
	if (pthread_join(r, NULL))
		abort();
	if (pthread_join(w, NULL))
		abort();
	assert(sum == 4 || sum == 6);
	return 0;
}
