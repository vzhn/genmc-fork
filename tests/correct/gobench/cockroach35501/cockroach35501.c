#include "../../../lib/go.h"

/*
 * GoBench kernel cockroach#35501 (non-blocking, anonymous function), after
 * CockroachDB's SchemaChanger.validateChecks(). For each check constraint
 * to validate, a goroutine validates it, given a pointer to the check's
 * name. Before Go 1.22, `for _, c := range checks` reuses one variable c,
 * which the loop overwrites while earlier goroutines read c.Name through
 * the pointer.
 *
 * BUG_SHARED_RANGE_VAR: the goroutines take &c.Name of the shared range
 * variable, as before CockroachDB PR #35501 (2019), which copies the check
 * into a per-iteration variable.
 */

#define NCHECKS 2

struct constraint_to_validate {
	int name;
};

struct constraint_to_validate checks[NCHECKS] = { { 1 }, { 2 } };
struct constraint_to_validate c;	/* the range variable */
struct constraint_to_validate copies[NCHECKS];
go_waitgroup_t wg;

void *validate_check_in_txn(void *arg)
{
	int *check_name = arg;

	assert(*check_name >= 1);	/* tableDesc.FindCheckByName(*checkName) */
	return NULL;
}

void *backfill(void *arg)
{
	for (int i = 0; i < NCHECKS; i++) {
#ifdef BUG_SHARED_RANGE_VAR
		c.name = checks[i].name;
		go(validate_check_in_txn, &c.name);
#else
		copies[i].name = checks[i].name;
		go(validate_check_in_txn, &copies[i].name);
#endif
	}
	go_wg_done(&wg);
	return NULL;
}

int main()
{
	go_init();
	go_wg_add(&wg, 1);
	go(backfill, NULL);
	go_wg_wait(&wg);
	return 0;
}
