/* Rust std before #98503: the last scoped thread unparks main through freed ScopeData */
#define BUG_SCOPE_DATA_ON_STACK
#include "../../../../correct/classic/std-thread-scope/std-thread-scope.c"
