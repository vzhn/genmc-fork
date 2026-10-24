#!/bin/bash

# Checks that GenMC rejects memory accesses whose values do not fit in 64 bits
#
# This program is dual-licensed under the Apache License 2.0 and the MIT License.
# You may choose to use, distribute, or modify this software under either license.
#
# Apache License 2.0:
#     http://www.apache.org/licenses/LICENSE-2.0
#
# MIT License:
#     https://opensource.org/licenses/MIT

# GenMC's values are 64 bits wide. A wider access whose value does not fit
# (an __int128 of 2^64 or more, or negative; a 16-byte atomic struct; a long
# double) used to be truncated, so a correct program failed its assertions.
# It is now a user error (status 17). A wide integer whose values fit still
# runs. ctest target "wide-accesses".
#
#   GenMC=<build>/bin/genmc scripts/check-wide-accesses.sh

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
GenMC="${GenMC:-$DIR/../RelWithDebInfo/bin/genmc}"
work=$(mktemp -d)
trap 'rm -rf "${work}"' EXIT
MSG='Memory accesses wider than 8 bytes are supported only for integers below 2^64'

status=0
run() { # <name> <expected status> <text> ; program on stdin
	cat > "${work}/$1.c"
	out=$("${GenMC}" -rc11 -disable-estimation -- "${work}/$1.c" < /dev/null 2>&1)
	rc=$?
	if [[ ${rc} -eq $2 ]] && echo "${out}" | grep -qF -- "$3"; then
		echo "ok   $1"
	else
		echo "FAIL $1: status ${rc}, expected $2 and \"$3\""; echo "${out}" | tail -3; status=1
	fi
}

run int128 17 "${MSG}: a 16-byte store (i128)" <<'PROG'
#include <assert.h>
#include <stdint.h>
volatile unsigned __int128 g;
int main() { g = ((unsigned __int128)7 << 64) | 5; assert((uint64_t)(g >> 64) == 7); return 0; }
PROG
run atomic-struct 17 "${MSG}: a 16-byte store" <<'PROG'
#include <assert.h>
#include <stdatomic.h>
struct pair { long a, b; };
_Atomic struct pair p;
int main() { struct pair v = {1, 2}; atomic_store(&p, v); assert(atomic_load(&p).b == 2); return 0; }
PROG
run long-double 17 "${MSG}: a 16-byte load (x86_fp80)" <<'PROG'
#include <assert.h>
volatile long double a = 1.5L;
int main() { long double c = a; assert(c > 1); return 0; }
PROG
run small-int128 0 "No errors were detected" <<'PROG'
#include <assert.h>
#include <stdatomic.h>
_Atomic(unsigned __int128) q = 5;
int main() { atomic_fetch_sub(&q, 2); assert(atomic_load(&q) == 3); return 0; }
PROG
exit ${status}
