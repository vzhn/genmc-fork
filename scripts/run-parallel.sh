#!/bin/bash

# Fast run of driver.sh with parallel exploration
#
# This program is dual-licensed under the Apache License 2.0 and the MIT License.
# You may choose to use, distribute, or modify this software under either license.
#
# Apache License 2.0:
#     http://www.apache.org/licenses/LICENSE-2.0
#
# MIT License:
#     https://opensource.org/licenses/MIT

# Runs the fast test suite with several exploration threads (ctest target
# "run-parallel"). NTHREADS (default: 4) sets the number of threads. The
# tests run in parallel too (scripts/parallel-driver.sh), JOBS at a time
# (default: one per NTHREADS cores).

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
source "${DIR}/terminal.sh"

NTHREADS="${NTHREADS:-4}"
JOBS="${JOBS:-$(( ($(nproc) + NTHREADS - 1) / NTHREADS ))}" \
	GENMCFLAGS="${GENMCFLAGS:-} -nthreads=${NTHREADS}" "${DIR}/parallel-driver.sh" --fast --debug
