# This file is part of BenchExec, a framework for reliable benchmarking:
# https://github.com/sosy-lab/benchexec
#
# SPDX-FileCopyrightText: 2007-2020 Dirk Beyer <https://www.sosy-lab.org>
#
# SPDX-License-Identifier: Apache-2.0

import benchexec.tools.template
from benchexec import result


class Tool(benchexec.tools.template.BaseTool2):
    """
    Tool info for GenMC (https://github.com/MPI-SWS/genmc), a stateless
    model checker for concurrent C programs under weak memory models, run on
    SV-COMP tasks through its wrapper genmc-svcomp (scripts/svcomp in GenMC's
    repository), which prints the result as its last line:
    "SV-COMP result: true", "false(unreach-call)", "false(no-data-race)",
    "false(valid-deref)", "false(valid-free)" or "unknown".
    """

    REQUIRED_PATHS = [
        "genmc",
        "svcomp_core.py",
        "svcomp_witness.py",
        "svcomp-shim.h",
        "svcomp-shim-pp.h",
        "svcomp-shim-core.h",
        "include",
    ]

    def executable(self, tool_locator):
        return tool_locator.find_executable("genmc-svcomp")

    def name(self):
        return "GenMC"

    def project_url(self):
        return "https://github.com/MPI-SWS/genmc"

    def version(self, executable):
        return self._version_from_tool(executable)

    def program_files(self, executable):
        return [executable] + self._program_files_from_executable(
            executable, self.REQUIRED_PATHS
        )

    def cmdline(self, executable, options, task, rlimits):
        data_model = (task.options or {}).get("data_model")
        if data_model:
            options = options + ["--data-model", data_model]
        return [executable] + options + [task.property_file] + list(task.input_files)

    def determine_result(self, run):
        results = {
            "true": result.RESULT_TRUE_PROP,
            "false(unreach-call)": result.RESULT_FALSE_REACH,
            "false(no-data-race)": result.RESULT_FALSE_DATARACE,
            "false(valid-deref)": result.RESULT_FALSE_DEREF,
            "false(valid-free)": result.RESULT_FALSE_FREE,
            "unknown": result.RESULT_UNKNOWN,
        }
        for line in reversed(run.output):
            if line.startswith("SV-COMP result: "):
                return results.get(line[len("SV-COMP result: ") :].strip(), result.RESULT_ERROR)
        return result.RESULT_ERROR
