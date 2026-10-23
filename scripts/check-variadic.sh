#!/bin/bash

# Checks that GenMC rejects variadic functions cleanly
#
# This program is dual-licensed under the Apache License 2.0 and the MIT License.
# You may choose to use, distribute, or modify this software under either license.
#
# Apache License 2.0:
#     http://www.apache.org/licenses/LICENSE-2.0
#
# MIT License:
#     https://opensource.org/licenses/MIT

# clang lowers va_arg() to loads from the target's va_list, which the
# interpreter does not lay out, so a function that calls va_start() is a
# user error (status 17) naming it. It used to abort LLVM (the inliner
# copied va_start() into a function that is not variadic: a broken
# module), and without inlining reported a false read of uninitialized
# memory. A variadic function that does not call va_start() runs (and is
# inlined). ctest target "variadic".
#
#   GenMC=<build>/bin/genmc scripts/check-variadic.sh

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
GenMC="${GenMC:-$DIR/../RelWithDebInfo/bin/genmc}"
work=$(mktemp -d)
trap 'rm -rf "${work}"' EXIT

status=0
cat > "${work}/sum.c" <<'PROG'
#include <assert.h>
#include <stdarg.h>
int sum(int n, ...)
{
	va_list ap;
	va_start(ap, n);
	int s = 0;
	for (int i = 0; i < n; i++)
		s += va_arg(ap, int);
	va_end(ap);
	return s;
}
int main() { assert(sum(2, 1, 2) == 3); return 0; }
PROG
out=$("${GenMC}" -rc11 -disable-estimation -- "${work}/sum.c" < /dev/null 2>&1)
rc=$?
if [[ ${rc} -eq 17 ]] && echo "${out}" | grep -q 'Variadic functions are not supported: sum()'; then
	echo "ok   va_start rejected"
else
	echo "FAIL va_start: status ${rc}, expected 17 and \"Variadic functions are not supported: sum()\""
	echo "${out}" | tail -3; status=1
fi

cat > "${work}/first.c" <<'PROG'
#include <assert.h>
int first(int n, ...) { return n; }
int main() { assert(first(5, 1.0, "x") == 5); return 0; }
PROG
out=$("${GenMC}" -rc11 -disable-estimation -- "${work}/first.c" < /dev/null 2>&1)
rc=$?
if [[ ${rc} -eq 0 ]] && echo "${out}" | grep -q 'No errors were detected'; then
	echo "ok   variadic without va_start runs"
else
	echo "FAIL variadic without va_start: status ${rc}"; echo "${out}" | tail -3; status=1
fi
exit ${status}
