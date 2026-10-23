#!/bin/bash
#
# #188: driver.sh runs every genmc under a memory limit, so that a test that
# runs away fails instead of exhausting the machine.
#
#  1. By default, genmc runs under the default limit (16000000 KiB).
#  2. tests/wrong/safety/condenser-reused-value under -sc has no bounding
#     arguments and grows without bound; with GENMC_MEMORY_LIMIT=1000000 the
#     run fails within the timeout, out of memory.
#
# A wrapper in place of genmc records the limit each run gets. The whole
# check runs under a 4 GB soft limit, so that it cannot exhaust the machine
# even when driver.sh sets no limit.
#
# This program is dual-licensed under the Apache License 2.0 and the MIT License.
# You may choose to use, distribute, or modify this software under either license.
#
# Apache License 2.0:
#     http://www.apache.org/licenses/LICENSE-2.0
#
# MIT License:
#     https://opensource.org/licenses/MIT

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
ROOT="${DIR}/.."
REAL="${GenMC:?set GenMC to the genmc binary of a build}"

T=$(mktemp -d)
trap 'rm -rf "${T}"' EXIT
mkdir -p "${T}/bin" "${T}/include/genmc" "${T}/empty" "${T}/correct" "${T}/wrong"
cp "$(dirname "$(dirname "${REAL}")")/include/genmc/config.h" "${T}/include/genmc/" || exit 1
cat > "${T}/bin/genmc" <<WRAPPER
#!/bin/bash
ulimit -S -v >> "${T}/limits"
exec "${REAL}" "\$@"
WRAPPER
chmod +x "${T}/bin/genmc"
status=0

# 1. The default limit, on a small correct test
cp -r "${ROOT}/tests/correct/litmus/2+2W" "${T}/correct/"
GenMC="${T}/bin/genmc" MODELS=rc11 CORRECT_DIRS="${T}/correct" WRONG_DIRS="${T}/empty" \
	"${ROOT}/scripts/driver.sh" --fast > "${T}/default.log" 2>&1
if [[ "$(sort -u "${T}/limits")" == "16000000" ]]; then
	echo "ok   default limit: 16000000 KiB"
else
	echo "FAIL: genmc ran under limit(s) '$(sort -u "${T}/limits" | tr '\n' ' ')', not the default 16000000"
	status=1
fi
rm -f "${T}/limits"

# 2. A runaway test fails, out of memory, under GENMC_MEMORY_LIMIT
ulimit -S -v 4000000
cp -r "${ROOT}/tests/wrong/safety/condenser-reused-value" "${T}/wrong/"
start=$(date +%s)
GENMC_MEMORY_LIMIT=1000000 GenMC="${T}/bin/genmc" MODELS=sc CORRECT_DIRS="${T}/empty" \
	WRONG_DIRS="${T}/wrong" timeout 120 "${ROOT}/scripts/driver.sh" --fast --debug > "${T}/runaway.log" 2>&1
rc=$?
took=$(($(date +%s) - start))
limits=$(sort -u "${T}/limits" | tr '\n' ' ')
if [[ ${rc} -eq 124 ]]; then
	echo "FAIL: the runaway test was still running after 120 s (limits: ${limits})"
	status=1
elif [[ "${limits}" != "1000000 " ]]; then
	echo "FAIL: genmc ran under limit(s) '${limits}', not GENMC_MEMORY_LIMIT=1000000"
	status=1
elif [[ ${rc} -eq 0 ]] || ! grep -q 'bad_alloc' "${T}/runaway.log"; then
	echo "FAIL: the runaway test did not fail out of memory (driver.sh status ${rc})"
	status=1
else
	echo "ok   runaway test failed out of memory under 1000000 KiB, in ${took} s"
fi
exit ${status}
