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
