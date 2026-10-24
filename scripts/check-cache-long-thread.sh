#!/bin/bash

# Checks that -cache-instructions is linear in a thread's length
#
# This program is dual-licensed under the Apache License 2.0 and the MIT License.
# You may choose to use, distribute, or modify this software under either license.
#
# Apache License 2.0:
#     http://www.apache.org/licenses/LICENSE-2.0
#
# MIT License:
#     https://opensource.org/licenses/MIT

# The cache used to re-read a thread's whole value prefix, and walk it
# from the root of its trie, for every label it added: O(L^2) for a
# thread of L events (issue #218). A thread of 20000 reads took 25 s
# with caching, 0.1 s without; it must now take about as long either
# way. (-check-invariants re-reads the prefix on purpose, so it is
# off.) ctest target "cache-long-thread".
#
#   GenMC=<build>/bin/genmc scripts/check-cache-long-thread.sh

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
GenMC="${GenMC:-$DIR/../RelWithDebInfo/bin/genmc}"
work=$(mktemp -d)
trap 'rm -rf "${work}"' EXIT

cat > "${work}/long.c" <<'PROG'
#include <pthread.h>
#include <stdatomic.h>
atomic_int x;
void *reader(void *arg)
{
	int s = 0;
	for (int i = 0; i < 20000; i++)
		s += atomic_load_explicit(&x, memory_order_relaxed);
	return (void *)(long)s;
}
int main()
{
	pthread_t t;
	pthread_create(&t, NULL, reader, NULL);
	pthread_join(t, NULL);
	return 0;
}
PROG
out=$(timeout 10 "${GenMC}" -rc11 -cache-instructions -check-invariants=false \
	-disable-estimation -- "${work}/long.c" < /dev/null 2>&1)
rc=$?
if [[ ${rc} -eq 0 ]] && echo "${out}" | grep -q 'Number of complete executions explored: 1$'; then
	echo "ok   a thread of 20000 reads is cached within 10 s"
	exit 0
fi
echo "FAIL a thread of 20000 reads with -cache-instructions: status ${rc} (124: over 10 s)"
echo "${out}" | tail -3
exit 1
