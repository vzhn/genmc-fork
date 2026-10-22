set(SCRIPT_DIR ${CMAKE_SOURCE_DIR}/scripts)
add_test(
    NAME fast-driver
    COMMAND bash -c "${SCRIPT_DIR}/fast-driver.sh > ${SCRIPT_DIR}/fast-driver.log"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
set_tests_properties(fast-driver PROPERTIES LABELS "integration;fast" TIMEOUT 600)

# The correct/ tests under -sc, which fast-driver leaves out
add_test(NAME sc-driver
    COMMAND bash -c "${SCRIPT_DIR}/sc-driver.sh"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
set_tests_properties(sc-driver PROPERTIES LABELS "integration;fast" TIMEOUT 600
    ENVIRONMENT "GenMC=${CMAKE_BINARY_DIR}/bin/genmc")

# The test scripts run a last configuration line without a newline too (#176)
add_test(NAME driver-last-line
    COMMAND bash -c "${SCRIPT_DIR}/check-driver-last-line.sh"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
set_tests_properties(driver-last-line PROPERTIES LABELS "integration;fast" TIMEOUT 120
    ENVIRONMENT "GenMC=${CMAKE_BINARY_DIR}/bin/genmc")

add_test(
    NAME randomize-driver
    COMMAND bash -c "${SCRIPT_DIR}/randomize-driver.sh > ${SCRIPT_DIR}/randomize-driver.log"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
set_tests_properties(randomize-driver PROPERTIES LABELS "integration" TIMEOUT 600)

add_test(NAME run-parallel
    COMMAND bash -c "${SCRIPT_DIR}/run-parallel.sh > ${SCRIPT_DIR}/run-parallel.log"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
set_tests_properties(run-parallel PROPERTIES LABELS "integration;concurrency" TIMEOUT 300)

add_test(NAME run-relinche-impl
    COMMAND bash -c "${SCRIPT_DIR}/run-relinche-impl.sh > ${SCRIPT_DIR}/run-relinche-impl.log"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
set_tests_properties(run-relinche-impl PROPERTIES LABELS "integration;relinche" TIMEOUT 300)

add_test(NAME run-relinche-spec
    COMMAND bash -c "${SCRIPT_DIR}/run-relinche-spec.sh > ${SCRIPT_DIR}/run-relinche-spec.log"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
set_tests_properties(run-relinche-spec PROPERTIES LABELS "integration;relinche" TIMEOUT 300)

if(SANITIZE MATCHES "address")
  set(_asan_env "ASAN_OPTIONS=detect_leaks=0:halt_on_error=0")
  set_tests_properties(fast-driver randomize-driver run-parallel PROPERTIES
      ENVIRONMENT "${_asan_env}")
endif()

# Data races between parallel exploration workers (ThreadSanitizer builds only)
if(SANITIZE MATCHES "thread")
  add_test(NAME tsan-parallel
      COMMAND bash -c "${SCRIPT_DIR}/tsan-parallel.sh > ${SCRIPT_DIR}/tsan-parallel.log"
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  )
  set_tests_properties(tsan-parallel PROPERTIES LABELS "integration;concurrency" TIMEOUT 1800
      ENVIRONMENT "GenMC=${CMAKE_BINARY_DIR}/bin/genmc")
endif()

# The default exploration against the -exhaustive reference, on a small subset
# of the suite (scripts/oracle-diff.sh runs any directories)
set(ORACLE_DIFF_TESTS
    tests/correct/litmus/MP tests/correct/litmus/SB tests/correct/litmus/LB
    tests/correct/litmus/2+2W tests/correct/litmus/CoRR tests/correct/litmus/isa2
    tests/correct/litmus/Z6.U
    tests/correct/infr/weak-cas-revisit-after-failure
    tests/correct/infr/weak-cas-backward-revisit-failure
    tests/correct/infr/weak-cas-fail-on-settled-rmw
    tests/correct/infr/join-retval tests/correct/infr/lock-inplace
    tests/correct/saver/assume-cas tests/correct/saver/spin-cas1
    tests/correct/ipr/lock-nested
    tests/correct/classic/dcl tests/correct/classic/counter
    tests/correct/data-structures/lamport-spsc)
string(REPLACE ";" " " ORACLE_DIFF_TESTS_ARGS "${ORACLE_DIFF_TESTS}")
add_test(NAME oracle-diff
    COMMAND bash -c "${SCRIPT_DIR}/oracle-diff.sh -j 2 ${CMAKE_BINARY_DIR}/bin/genmc ${ORACLE_DIFF_TESTS_ARGS} > ${SCRIPT_DIR}/oracle-diff.log"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
set_tests_properties(oracle-diff PROPERTIES LABELS "integration;fast" TIMEOUT 600)

add_test(NAME liveness-tip
    COMMAND bash -c "${SCRIPT_DIR}/check-liveness-tip.sh"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
set_tests_properties(liveness-tip PROPERTIES LABELS "integration;fast" TIMEOUT 120
    ENVIRONMENT "GenMC=${CMAKE_BINARY_DIR}/bin/genmc")

add_test(NAME lkmm-ww-warning
    COMMAND bash -c "${SCRIPT_DIR}/check-lkmm-ww-warning.sh"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
set_tests_properties(lkmm-ww-warning PROPERTIES LABELS "integration;fast" TIMEOUT 120
    ENVIRONMENT "GenMC=${CMAKE_BINARY_DIR}/bin/genmc")

add_test(NAME race-detection-model
    COMMAND bash -c "${SCRIPT_DIR}/check-race-detection-model.sh"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
set_tests_properties(race-detection-model PROPERTIES LABELS "integration;fast" TIMEOUT 120
    ENVIRONMENT "GenMC=${CMAKE_BINARY_DIR}/bin/genmc")

add_test(NAME nondet-warning
    COMMAND bash -c "${SCRIPT_DIR}/check-nondet-warning.sh"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
set_tests_properties(nondet-warning PROPERTIES LABELS "integration;fast" TIMEOUT 120
    ENVIRONMENT "GenMC=${CMAKE_BINARY_DIR}/bin/genmc")

add_test(NAME install-headers
    COMMAND bash -c "${SCRIPT_DIR}/check-install-headers.sh"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
set_tests_properties(install-headers PROPERTIES LABELS "integration;fast" TIMEOUT 300
    ENVIRONMENT "BUILD=${CMAKE_BINARY_DIR}")

# The SV-COMP preludes build against the runtime headers (needs PyYAML; skipped without it)
add_test(NAME svcomp-shim
    COMMAND bash -c "${SCRIPT_DIR}/check-svcomp-shim.sh"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
set_tests_properties(svcomp-shim PROPERTIES LABELS "integration;fast" TIMEOUT 300
    SKIP_RETURN_CODE 77 ENVIRONMENT "GenMC=${CMAKE_BINARY_DIR}/bin/genmc")

add_test(NAME liveness-report
    COMMAND bash -c "${SCRIPT_DIR}/check-liveness-report.sh"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
set_tests_properties(liveness-report PROPERTIES LABELS "integration;fast" TIMEOUT 300
    ENVIRONMENT "GenMC=${CMAKE_BINARY_DIR}/bin/genmc")

add_test(NAME call-site-locs
    COMMAND bash -c "${SCRIPT_DIR}/check-call-site-locs.sh"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
set_tests_properties(call-site-locs PROPERTIES LABELS "integration;fast" TIMEOUT 120
    ENVIRONMENT "GenMC=${CMAKE_BINARY_DIR}/bin/genmc")

add_test(NAME invariants-fast
    COMMAND bash -c "${SCRIPT_DIR}/check-invariants-fast.sh"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
set_tests_properties(invariants-fast PROPERTIES LABELS "integration;fast" TIMEOUT 600
    ENVIRONMENT "GenMC=${CMAKE_BINARY_DIR}/bin/genmc")

add_test(NAME unordered-writes-fallback
    COMMAND bash -c "${SCRIPT_DIR}/check-unordered-writes-fallback.sh"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
set_tests_properties(unordered-writes-fallback PROPERTIES LABELS "integration;fast" TIMEOUT 300
    ENVIRONMENT "GenMC=${CMAKE_BINARY_DIR}/bin/genmc")

# The passes keep the dependencies of LKMM programs (needs alive-tv; skipped without it)
add_test(NAME check-pass-deps
    COMMAND bash -c "${SCRIPT_DIR}/check-pass-deps.sh"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
set_tests_properties(check-pass-deps PROPERTIES LABELS "integration" TIMEOUT 1800
    SKIP_RETURN_CODE 77 ENVIRONMENT "GenMC=${CMAKE_BINARY_DIR}/bin/genmc")

# Unit tests
add_subdirectory(unit)

# TLA+ models of the error replay and the worker pool (models/tla): the fixed
# configurations must pass, the bug configurations must reproduce their bug
find_program(TLC_EXECUTABLE tlc)
if(TLC_EXECUTABLE)
  add_test(NAME tla-models
      COMMAND bash -c "${SCRIPT_DIR}/check-tla.sh -j 2"
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  )
  set_tests_properties(tla-models PROPERTIES LABELS "models" TIMEOUT 1800
      ENVIRONMENT "TLC=${TLC_EXECUTABLE}")
endif()
