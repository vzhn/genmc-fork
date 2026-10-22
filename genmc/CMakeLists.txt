# The core library, one module per layer. A module may use the modules
# below it, never those above (scripts/check-core-layering.sh checks the
# includes; the genmc_layer_check_* executables below check the links).
#
#   genmc_support       ADT/, Support/: containers, clocks, values, errors
#   genmc_graph         Execution/: events, labels, graphs, the GraphModel
#                       interface the graph needs from a memory model
#   genmc_models        Models/: the memory models (Kater
#                       generated), symmetry and bounding deciders
#   genmc_verification  Verification/: exploration, driver, configuration,
#                       Relinche
#
# genmc_lib is all of them in one archive (what the tool, the passes and the
# tests link, and what gets installed).

set(GENMC_SUPPORT_SOURCES
  genmc/ADT/AdaptiveView.cpp
  genmc/ADT/DepView.cpp
  genmc/ADT/SmallVector.cpp
  genmc/ADT/VectorClock.cpp
  genmc/ADT/View.cpp
  genmc/Support/Error.cpp
  genmc/Support/NameInfo.cpp
  genmc/Support/Parser.cpp
  genmc/Support/SAddrAllocator.cpp
  genmc/Support/SVal.cpp
  genmc/Support/ThreadPinner.cpp
)
set(GENMC_GRAPH_SOURCES
  genmc/Execution/CBIterator.cpp
  genmc/Execution/DepExecutionGraph.cpp
  genmc/Execution/EventLabel.cpp
  genmc/Execution/ExecutionGraph.cpp
  genmc/Execution/ExecutionState.cpp
  genmc/Execution/GraphUtils.cpp
  genmc/Execution/ModelInterfaces.cpp
)
set(GENMC_MODELS_SOURCES
  genmc/Models/LkmmModel.cpp
  genmc/Models/LkmmRelations.cpp
  genmc/Models/BoundDecider.cpp
  genmc/Models/ConsistencyChecker.cpp
  genmc/Models/ContextBoundDecider.cpp
  genmc/Models/IMMChecker.cpp
  genmc/Models/LKMMChecker.cpp
  genmc/Models/LKMMRcuChecker.cpp
  genmc/Models/RAChecker.cpp
  genmc/Models/RC11Checker.cpp
  genmc/Models/RoundBoundDecider.cpp
  genmc/Models/SCChecker.cpp
  genmc/Models/SymmetryChecker.cpp
  genmc/Models/TSOChecker.cpp
)
set(GENMC_VERIFICATION_SOURCES
  genmc/Verification/Rules/ChoiceCandidates.cpp
  genmc/Verification/State/ChoiceMap.cpp
  genmc/Verification/Config.cpp
  genmc/Verification/Rules/ErrorRules.cpp
  genmc/Verification/State/ExecutionFrame.cpp
  genmc/Verification/Rules/ExhaustiveConsistency.cpp
  genmc/Verification/Core/Explorer.cpp
  genmc/Verification/Core/ExplorationCore.cpp
  genmc/Verification/State/ExplorationStack.cpp
  genmc/Verification/Core/GenMCDriver.cpp
  genmc/Verification/Checks/GraphHash.cpp
  genmc/Verification/Report/GraphPrinting.cpp
  genmc/Verification/Checks/Invariants.cpp
  genmc/Verification/Relinche/LinearizabilityChecker.cpp
  genmc/Verification/Relinche/Linearization.cpp
  genmc/Verification/Relinche/Observation.cpp
  genmc/Verification/Relinche/Specification.cpp
  genmc/Verification/State/ResumePointStore.cpp
  genmc/Verification/Rules/RfReductions.cpp
  genmc/Verification/Rules/BackwardRevisits.cpp
  genmc/Verification/State/Scheduler.cpp
  genmc/Verification/Rules/SymmetricThreads.cpp
)

find_package(Threads REQUIRED)
find_library(HWLOC hwloc)
if(NOT HWLOC)
  message(STATUS "hwloc not found. Thread pinning disabled.")
endif()

set(GENMC_MODULES genmc_support genmc_graph genmc_models genmc_verification)
add_library(genmc_support OBJECT ${GENMC_SUPPORT_SOURCES})
add_library(genmc_graph OBJECT ${GENMC_GRAPH_SOURCES})
add_library(genmc_models OBJECT ${GENMC_MODELS_SOURCES})
add_library(genmc_verification OBJECT ${GENMC_VERIFICATION_SOURCES})
foreach(_mod IN LISTS GENMC_MODULES)
  target_include_directories(${_mod} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
  target_compile_options(${_mod} PRIVATE ${GENMC_WARNING_FLAGS})
  target_link_libraries(${_mod} PUBLIC Threads::Threads genmc_config_includes)
endforeach()
if(HWLOC)
  target_link_libraries(genmc_support PRIVATE hwloc)
endif()

add_library(genmc_lib ${GENMC_LIB_TYPE}
  $<TARGET_OBJECTS:genmc_support> $<TARGET_OBJECTS:genmc_graph>
  $<TARGET_OBJECTS:genmc_models> $<TARGET_OBJECTS:genmc_verification>)
target_include_directories(genmc_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(genmc_lib PUBLIC Threads::Threads genmc_config_includes)
if(HWLOC)
  target_link_libraries(genmc_lib PRIVATE hwloc)
endif()
install(TARGETS genmc_lib ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME})
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/genmc/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp" PATTERN "*.def")

# Link checks: each module, linked whole with only the modules below it,
# must have no undefined symbols (a call into a module above fails here)
set(_below "")
foreach(_mod IN LISTS GENMC_MODULES)
  list(APPEND _below $<TARGET_OBJECTS:${_mod}>)
  add_executable(genmc_layer_check_${_mod} ${CMAKE_CURRENT_SOURCE_DIR}/layer-check-main.cpp ${_below})
  target_link_libraries(genmc_layer_check_${_mod} PRIVATE Threads::Threads genmc_config_includes)
  if(HWLOC)
    target_link_libraries(genmc_layer_check_${_mod} PRIVATE hwloc)
  endif()
endforeach()
