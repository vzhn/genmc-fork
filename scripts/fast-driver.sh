#!/bin/bash

# A fast run of driver.sh's tests: in parallel, one job per model and test
# category (scripts/parallel-driver.sh, JOBS at a time), or as one driver.sh
# with FAST_DRIVER_SERIAL=1, or when CORRECT_DIRS or WRONG_DIRS choose the
# directories
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
source "${DIR}/terminal.sh"

if [[ -n "${FAST_DRIVER_SERIAL:-}" || -n "${CORRECT_DIRS+x}" || -n "${WRONG_DIRS+x}" ]]; then
	"${DIR}/driver.sh" --fast --debug
else
	"${DIR}/parallel-driver.sh" --fast --debug
fi
