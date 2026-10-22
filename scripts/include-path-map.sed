# Maps the paths of genmc/genmc files that moved to their new paths, with or
# without the genmc/ (or genmc/genmc/) prefix: includes, CMake source lists,
# scripts and comments. To update a branch that predates the move:
#
#   git grep -lE '(Execution|Verification)/' -- . | xargs sed -i -E -f scripts/include-path-map.sed
#
# This program is dual-licensed under the Apache License 2.0 and the MIT License.
# You may choose to use, distribute, or modify this software under either license.

# models
s#Execution/Consistency/ModelInterfaces\.#Execution/ModelInterfaces.#g
s#Execution/LkmmModel\.#Models/LkmmModel.#g
s#Execution/LkmmRelations\.#Models/LkmmRelations.#g
s#Execution/Consistency/#Models/#g
s#Execution/Consistency\b#Models#g

# verification
s#Verification/GraphPrinting\.#Verification/Report/GraphPrinting.#g
s#Verification/DebugInfo\.#Verification/Report/DebugInfo.#g
s#Verification/Invariants\.#Verification/Checks/Invariants.#g
s#Verification/GraphHash\.#Verification/Checks/GraphHash.#g
s#Verification/GraphFingerprint\.#Verification/Checks/GraphFingerprint.#g
s#Verification/ExplorationStack\.#Verification/State/ExplorationStack.#g
s#Verification/ExecutionFrame\.#Verification/State/ExecutionFrame.#g
s#Verification/ResumePointStore\.#Verification/State/ResumePointStore.#g
s#Verification/ResumePoint\.#Verification/State/ResumePoint.#g
s#Verification/KeptEventRuns\.#Verification/State/KeptEventRuns.#g
s#Verification/ErrorReplay\.#Verification/State/ErrorReplay.#g
s#Verification/Scheduler\.#Verification/State/Scheduler.#g
s#Verification/WorkList\.#Verification/State/WorkList.#g
s#Verification/Revisit\.#Verification/State/Revisit.#g
s#Verification/ChoiceMap\.#Verification/State/ChoiceMap.#g
s#Verification/BackwardRevisits\.#Verification/Rules/BackwardRevisits.#g
s#Verification/RfReductions\.#Verification/Rules/RfReductions.#g
s#Verification/ChoiceCandidates\.#Verification/Rules/ChoiceCandidates.#g
s#Verification/ExhaustiveConsistency\.#Verification/Rules/ExhaustiveConsistency.#g
s#Verification/ErrorRules\.#Verification/Rules/ErrorRules.#g
s#Verification/SymmetricThreads\.#Verification/Rules/SymmetricThreads.#g
s#Verification/ExplorationCore\.#Verification/Core/ExplorationCore.#g
s#Verification/GenMCDriver\.#Verification/Core/GenMCDriver.#g
s#Verification/DriverHandlerDispatcher\.#Verification/Core/DriverHandlerDispatcher.#g
s#Verification/Explorer\.#Verification/Core/Explorer.#g
s#Verification/EventProducer\.#Verification/Core/EventProducer.#g
s#Verification/WorkSharing\.#Verification/Core/WorkSharing.#g
