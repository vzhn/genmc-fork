#!/bin/bash
#
# Runs scripts/driver.sh's tests as one job per model and test category,
# JOBS at a time, and prints the final counts: one row per kind (correct or
# wrong tests), model and category. Exits non-zero if a job fails, after
# printing its log.
#
# The categories are driver.sh's defaults; MODELS (default: rc11 imm lkmm),
# GenMC, GENMCFLAGS and GENMC_MEMORY_LIMIT apply as in driver.sh, and the
# arguments (e.g. --fast, --debug) go to every driver.sh. The slowest
# category, synthetic, runs in three parts by test name.
#
# Usage: JOBS=8 scripts/parallel-driver.sh --fast
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
ROOT="$(cd "${DIR}/.." && pwd)"
JOBS="${JOBS:-$(nproc)}"
MODELS=(${MODELS:-rc11 imm lkmm})
CORRECT=(infr litmus saver ipr sr liveness synthetic data-structures classic gobench lkmm)
WRONG=(safety liveness infr racy memory locking barriers helper)
# the parts of a category run as several jobs (patterns on test names)
SYNTHETIC_PARTS=('[a-bA-B]*' '[c-oC-O]*' '[p-zP-Z]*')

work=$(mktemp -d)
trap 'rm -rf "${work}"' EXIT
mkdir "${work}/empty" "${work}/logs"
start=$(date +%s)

# The jobs, slowest categories first: kind model category [name pattern]
for m in "${MODELS[@]}"; do
	for p in "${SYNTHETIC_PARTS[@]}"; do echo "correct ${m} synthetic ${p}"; done
done > "${work}/list"
for c in gobench infr data-structures classic saver litmus ipr sr liveness lkmm; do
	for m in "${MODELS[@]}"; do echo "correct ${m} ${c}"; done
done >> "${work}/list"
for c in "${WRONG[@]}"; do
	for m in "${MODELS[@]}"; do echo "wrong ${m} ${c}"; done
done >> "${work}/list"

run_job() {
	local kind=$1 model=$2 category=$3 pattern=${4:-*}
	local name="${kind}-${model}-${category}${4:+-$(echo "${pattern}" | md5sum | cut -c1-6)}"
	local correct="${work}/empty" wrong="${work}/empty"
	if [[ "${kind}" == correct ]]; then
		correct="${ROOT}/tests/correct/${category}"
	else
		wrong="${ROOT}/tests/wrong/${category}"
	fi
	CORRECT_DIRS="${correct}" WRONG_DIRS="${wrong}" MODELS="${model}" TESTFILTER="${pattern}" \
		"${DIR}/driver.sh" "${args[@]}" > "${work}/logs/${name}.log" 2>&1
	local status=$?
	grep -q 'Testing proceeded as expected' "${work}/logs/${name}.log" || status=1
	echo "${status} ${kind} ${model} ${category} ${name}" >> "${work}/status"
}
args=("$@")
export -f run_job
export work ROOT DIR
export args_string="${args[*]}"
xargs -P "${JOBS}" -L 1 bash -c 'args=(${args_string}); run_job "$@"' _ < "${work}/list"

# The tests a job ran: without --debug, the counts of its last row for its
# model and category (driver.sh reprints the row after every test); with
# --debug, its rows of single tests. The parts of a category add up.
tests_of() {
	sed 's/\x1b\[[0-9;]*[a-zA-Z]//g' -- "$1" | tr '\r' '\n' | grep -E '^\| ' | awk -F'|' '
		{gsub(/ /, "")}
		$2 ~ /^[A-Z0-9]+-(MO|WB)$/ {summary = $4 + $5; next}
		$2 != "" && $2 != "Testcase" && $2 != "Model" {single++}
		END {print (summary != "" ? summary : single + 0)}'
}
printf '%-8s %-6s %-16s %6s  %s\n' kind model category tests result
while read -r status kind model category name; do
	echo "${kind} ${model} ${category} $(tests_of "${work}/logs/${name}.log") ${status}"
done < "${work}/status" |
	awk '{k = $1" "$2" "$3; n[k] += $4; if ($5 != 0) bad[k] = 1}
	     END {for (k in n) {split(k, f, " "); printf "%-8s %-6s %-16s %6d  %s\n", f[1], f[2], f[3], n[k], bad[k] ? "FAIL" : "ok"}}' | sort

failed=$(awk '$1 != 0 {print $5}' "${work}/status")
total=$(($(date +%s) - start))
if [[ -n "${failed}" ]]; then
	for name in ${failed}; do
		echo
		echo "=== failed: ${name}"
		sed 's/\x1b\[[0-9;]*[a-zA-Z]//g' -- "${work}/logs/${name}.log" | tr '\r' '\n' | grep -vE '^\s*$'
	done
	echo "--- UNEXPECTED TESTING RESULTS ($(echo ${failed} | wc -w) of $(wc -l < "${work}/list") jobs failed). Total time: ${total}"
	exit 1
fi
echo "--- Testing proceeded as expected ($(wc -l < "${work}/list") jobs, ${JOBS} at a time). Total time: ${total}"
