#!/bin/bash

# Checks that an installed GenMC is self-contained
#
# This program is dual-licensed under the Apache License 2.0 and the MIT License.
# You may choose to use, distribute, or modify this software under either license.
#
# Apache License 2.0:
#     http://www.apache.org/licenses/LICENSE-2.0
#
# MIT License:
#     https://opensource.org/licenses/MIT

# Installs the build into a temporary prefix and checks that the installed
# binary compiles programs with the runtime headers installed with it alone:
# every header of lli/runtime-include/c is installed (the C++ ones have no
# extension), a marker added to the installed pthread.h is seen although
# the source tree's pthread.h comes first for a build-tree binary, a C++
# program with <atomic> and <cassert> verifies, and all of it still holds
# once the prefix is moved. ctest target "install-headers".
#
#   BUILD=<build> scripts/check-install-headers.sh

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
BUILD="${BUILD:-$DIR/../RelWithDebInfo}"
SRC="${DIR}/../lli/runtime-include/c"
MARK=GENMC_CHECK_INSTALLED_HEADERS

tmp=$(mktemp -d "${TMPDIR:-/tmp}/genmc-install.XXXXXX")
trap 'rm -rf "${tmp}"' EXIT

if ! cmake --install "${BUILD}" --prefix "${tmp}/prefix" > "${tmp}/install.log" 2>&1; then
	cat "${tmp}/install.log"
	echo "FAIL: cmake --install ${BUILD}"
	exit 1
fi
inc="${tmp}/prefix/include/genmc/runtime"

status=0
fail() {
	echo "FAIL: $*"
	status=1
}

for f in "${SRC}"/*; do
	h=$(basename "${f}")
	[[ -e "${inc}/${h}" ]] || fail "runtime header ${h} not installed"
done

echo "#define ${MARK} 1" >> "${inc}/pthread.h"
cat > "${tmp}/probe.c" <<PROBE
#include <pthread.h>
#ifndef ${MARK}
#error "the runtime headers are not the ones installed with genmc"
#endif
int main(void) { return 0; }
PROBE
cat > "${tmp}/probe.cpp" <<PROBE
#include <atomic>
#include <cassert>
#include <pthread.h>

std::atomic<int> x{0};

void *t(void *)
{
	x.fetch_add(1, std::memory_order_relaxed);
	return nullptr;
}

int main()
{
	pthread_t h;
	pthread_create(&h, nullptr, t, nullptr);
	x.fetch_add(1, std::memory_order_relaxed);
	pthread_join(h, nullptr);
	assert(x.load() == 2);
	return 0;
}
PROBE

check() {
	local where=$1 genmc=$2
	local out
	out=$("${genmc}" -disable-estimation -- "${tmp}/probe.c" 2>&1)
	grep -q "explored: 1$" <<< "${out}" ||
		fail "${where}: not compiled with the installed headers:"$'\n'"${out}"
	out=$("${genmc}" -disable-estimation -- "${tmp}/probe.cpp" 2>&1)
	grep -q "explored: 2$" <<< "${out}" ||
		fail "${where}: C++ program with <atomic> and <cassert>:"$'\n'"${out}"
}

check "installed" "${tmp}/prefix/bin/genmc"
mv "${tmp}/prefix" "${tmp}/moved"
check "moved prefix" "${tmp}/moved/bin/genmc"

[[ ${status} -eq 0 ]] && echo "PASS: the installed genmc uses the headers installed with it"
exit ${status}
