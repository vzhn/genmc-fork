include(FetchContent)

# The test libraries are for the tests only: `cmake --install` leaves them
# out (EXCLUDE_FROM_ALL drops their install rules; INSTALL_GTEST for a CMake
# before 3.28, which ignores it)

# Fetch googletest
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
  EXCLUDE_FROM_ALL
)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
# set(CMAKE_CXX_STANDARD 20)
FetchContent_MakeAvailable(googletest)

# Fetch rapidcheck (as an archive: CI runners choke on git-over-HTTPS)
FetchContent_Declare(
  rapidcheck
  URL https://github.com/emil-e/rapidcheck/archive/6e8dadfdafa3a74eabb52ead87f8787f72eccd0b.zip
  EXCLUDE_FROM_ALL
)
set(RC_ENABLE_GTEST ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(rapidcheck)

add_executable(unit_tests IntervalMapTest.cpp IntervalSetTest.cpp ViewTest.cpp ErrorTest.cpp CastTest.cpp CacheCounterTest.cpp ExecutionGraphTest.cpp RelocateTest.cpp RMWOpsTest.cpp SExprEvaluatorTest.cpp GraphHashTest.cpp RaceTest.cpp AdaptiveViewTest.cpp InvariantsTest.cpp TrieTest.cpp BitVectorTest.cpp VSetTest.cpp GraphPropertyTest.cpp)
# LLVM: the reference semantics of RMWOpsTest and SExprEvaluatorTest (llvm::APInt)
target_link_libraries(unit_tests PRIVATE GTest::gtest_main rapidcheck_gtest genmc_config_includes genmc_lib LLVM)
target_include_directories(unit_tests PRIVATE ${CMAKE_SOURCE_DIR}/genmc)
target_include_directories(unit_tests SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS})
target_compile_definitions(unit_tests PRIVATE ENABLE_GENMC_DEBUG) # to access validate()

# The exploration without LLVM: a scripted event producer drives the core
# library alone (no ENABLE_GENMC_DEBUG: the library is built without it, and
# Config must have the same layout on both sides)
add_executable(driver_tests driver/DriverTest.cpp driver/ErrorRulesTest.cpp driver/ExplorationStackTest.cpp driver/ResumePointStoreTest.cpp driver/KeptEventRunsTest.cpp driver/KeptEventAlignmentTest.cpp driver/SpinloopRulesTest.cpp driver/RevisitOptimizationsTest.cpp driver/RevisitTest.cpp driver/ConfigTest.cpp driver/SymmetricThreadsTest.cpp)
target_link_libraries(driver_tests PRIVATE GTest::gtest_main rapidcheck_gtest genmc_config_includes genmc_lib)
target_include_directories(driver_tests PRIVATE ${CMAKE_SOURCE_DIR}/genmc ${CMAKE_CURRENT_SOURCE_DIR}/driver)

# register with ctest
include(GoogleTest)
gtest_discover_tests(unit_tests)
gtest_discover_tests(driver_tests)

# Validation as part of the build: whenever the test binaries are relinked
# (a change to the library relinks them), `ninja` (target "all") runs the
# unit and driver tests, the graph properties with more cases than by
# default, and fails the build if one fails. A property failure prints the
# shrunk case and RC_PARAMS to reproduce it. -DGENMC_VALIDATE_ON_BUILD=OFF
# leaves the tests to ctest.
option(GENMC_VALIDATE_ON_BUILD "Run the unit and driver tests as part of the build" ON)
if(GENMC_VALIDATE_ON_BUILD)
  set(_validate_stamp ${CMAKE_CURRENT_BINARY_DIR}/validate.stamp)
  add_custom_command(OUTPUT ${_validate_stamp}
    COMMAND ${CMAKE_COMMAND} -E env RC_PARAMS=max_success=1000
            $<TARGET_FILE:unit_tests> --gtest_brief=1
    COMMAND $<TARGET_FILE:driver_tests> --gtest_brief=1
    COMMAND ${CMAKE_COMMAND} -E touch ${_validate_stamp}
    DEPENDS unit_tests driver_tests
    COMMENT "Validating: unit and driver tests (graph properties: 1000 cases)"
    VERBATIM)
  add_custom_target(validate ALL DEPENDS ${_validate_stamp})
endif()
