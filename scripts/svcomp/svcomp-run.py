#!/usr/bin/env python3
#
# This program is dual-licensed under the Apache License 2.0 and the MIT License.
# You may choose to use, distribute, or modify this software under either license.

"""Run GenMC on SV-COMP tasks and compare its result with the expected verdicts.

Usage: svcomp-run.py [-j JOBS] [-t TIMEOUT] [-m MODEL] [-o OUTDIR]
                     [--nondet-max N] [--max-choices N] [--max-spurious N]
                     [--witness] GENMC TASK_DIR...

For each <task>.yml, runs GenMC on the task's input file (input_files, which
for most tasks is a preprocessed .i) with svcomp-shim.h and maps the outcome to the properties GenMC can decide:
  unreach-call    false if GenMC reports a safety violation (failed assertion)
  no-data-race    false if GenMC reports a non-atomic race
  valid-memsafety false if GenMC reports a memory error
GenMC stops at the first error, so a task whose verdict is false for one
property is only checked for that property.

A clean result with blocked executions is checked again with the shim's
bounds check on (svcomp-shim-core.h): if an execution may have been cut by
the shim's bounds, the task is labelled BOUNDED and gets no verdict.

A race where one side lies inside an atomic section is labelled ATOMIC-RACE
and gets no verdict: SV-COMP counts accesses in atomic sections as atomic,
while the shim implements atomic sections with a lock. A race on the store of
a new thread's ID is labelled CREATE-RACE and gets no verdict: glibc's
pthread_create stores the ID before the thread runs, GenMC's after. The
shim's pthread_create stores it atomically, which tells that store from
the task's own.

GenMC runs with -svcomp where it has the option, so that a read of
uninitialized memory, which SV-COMP does not count as a memory-safety
violation, reads 0; older releases report such reads, which get no verdict.

Tasks that include <pthread.h> get svcomp-shim.h. Preprocessed tasks, which
declare the pthread API themselves, get svcomp-shim-pp.h and a copy of the task
with a generated tail that defines the pthread functions the task mentions,
using the task's own types.

Writes OUTDIR/results.tsv and one OUTDIR/<dir>/<task>.out per task; with
--witness, also a violation witness for each error GenMC finds, in GraphML
1.0 and YAML 2.2, as OUTDIR/<dir>/<task>.witness.{graphml,yml}
(svcomp_witness.py).
"""
import argparse
import concurrent.futures as cf
import os
import time

import yaml

import svcomp_witness
from svcomp_core import PROPS, genmc_options, specification, svcomp_result, verify

def verdict(expected, got):
    falses = [k for k, v in expected.items() if v is False]
    if got == "ok":
        return "AGREE" if not falses else "MISSED"
    if got in PROPS:
        if got in falses:
            return "AGREE"
        if expected.get(got) is True:
            return "FALSE-ALARM"
        return "UNCHECKED"
    return got.upper()


def run_task(args, d, yml):
    task = yaml.safe_load(open(os.path.join(d, yml)))
    expected = {p["property_file"].split("/")[-1][:-4]: p.get("expected_verdict")
                for p in task.get("properties", [])}
    expected = {k: v for k, v in expected.items() if k in PROPS}
    if not expected:
        return None
    name = yml[:-4]
    # SV-COMP verifies the task's input file, which for most tasks is the
    # preprocessed .i, not the .c it was made from
    inputs = task.get("input_files", [])
    inputs = [inputs] if isinstance(inputs, str) else inputs
    src = os.path.join(d, inputs[0]) if len(inputs) == 1 else ""
    if not os.path.exists(src):
        return (os.path.basename(d), name, expected, "no-input", 0.0, "NO-INPUT", "")
    od = os.path.join(args.outdir, os.path.basename(d))
    os.makedirs(od, exist_ok=True)
    if args.per_property:
        return [run_property(args, d, od, task, name, src, prop, exp)
                for prop, exp in sorted(expected.items())]
    t0 = time.time()
    got, out, compiled = verify(args.genmc, src, od, set(expected), args.timeout, args.model,
                                args.nondet_max, args.max_choices, args.max_spurious,
                                args.options,
                                args.memlimit)
    if args.witness and got in PROPS:
        prp = next(p["property_file"] for p in task["properties"]
                   if p["property_file"].endswith("/" + got + ".prp"))
        svcomp_witness.write_witnesses(out, os.path.basename(compiled), src,
                                       specification(got, out, os.path.join(d, prp)),
                                       task.get("options", {}).get("data_model", "ILP32"),
                                       os.path.join(od, name + ".witness"))
    dt = time.time() - t0
    with open(os.path.join(od, name + ".out"), "w") as f:
        f.write(out)
    first = next((l for l in out.splitlines()
                  if l.startswith(("Error", "ERROR")) or "error:" in l), "")
    return (os.path.basename(d), name, expected, got, dt, verdict(expected, got), first)


def pair_verdict(expected, res, got):
    """The verdict of one task/property pair from SV-COMP's result RES"""
    if res == "true":
        return "AGREE" if expected else "MISSED"
    if res.startswith("false"):
        return "FALSE-ALARM" if expected else "AGREE"
    # (no verdict: say why, as for whole tasks; UNFREED: valid-memsafety
    # with memory left unfreed, which may be reachable still)
    return "UNCHECKED" if got in PROPS else "UNFREED" if got == "ok" else got.upper()


def run_property(args, d, od, task, name, src, prop, exp):
    """Checks PROP alone, as the SV-COMP wrapper genmc-svcomp does"""
    pod = os.path.join(od, name + "." + prop)
    os.makedirs(pod, exist_ok=True)
    t0 = time.time()
    got, out, compiled = verify(args.genmc, src, pod, {prop}, args.timeout, args.model,
                                args.nondet_max, args.max_choices, args.max_spurious,
                                args.options,
                                args.memlimit)
    res = svcomp_result(prop, got, out)
    if args.witness and res.startswith("false"):
        prp = next(p["property_file"] for p in task["properties"]
                   if p["property_file"].endswith("/" + prop + ".prp"))
        svcomp_witness.write_witnesses(out, os.path.basename(compiled), src,
                                       specification(got, out, os.path.join(d, prp)),
                                       task.get("options", {}).get("data_model", "ILP32"),
                                       os.path.join(pod, "witness"))
    dt = time.time() - t0
    with open(os.path.join(pod, "out"), "w") as f:
        f.write(out)
    first = next((l for l in out.splitlines()
                  if l.startswith(("Error", "ERROR")) or "error:" in l), "")
    return (os.path.basename(d), name + ":" + prop, {prop: exp}, res, dt,
            pair_verdict(exp, res, got), first)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("-j", type=int, default=8)
    ap.add_argument("-t", "--timeout", type=int, default=60)
    ap.add_argument("-m", "--model", default="rc11")
    ap.add_argument("-o", "--outdir", default="svcomp-results")
    ap.add_argument("--nondet-max", type=int, default=2,
                    help="nondet values range over 0..N (default 2)")
    ap.add_argument("--max-choices", type=int, default=4,
                    help="nondet values per execution (default 4)")
    ap.add_argument("--max-spurious", type=int, default=1,
                    help="spurious condvar wake-ups per execution (default 1)")
    ap.add_argument("--witness", action="store_true",
                    help="write OUTDIR/<dir>/<task>.witness.{graphml,yml} for each error")
    ap.add_argument("--memlimit", type=float,
                    help="GB of address space for each GenMC run (no limit by default)")
    ap.add_argument("--per-property", action="store_true",
                    help="check each property of a task alone, as SV-COMP does, "
                    "with a verdict per task/property pair")
    ap.add_argument("genmc")
    ap.add_argument("dirs", nargs="+")
    args = ap.parse_args()
    args.options = genmc_options(args.genmc)
    args.memlimit = int(args.memlimit * 2**30) if args.memlimit else None

    jobs = [(d, y) for d in args.dirs for y in sorted(os.listdir(d)) if y.endswith(".yml")]
    rows = []
    with cf.ThreadPoolExecutor(args.j) as ex:
        for r in ex.map(lambda j: run_task(args, *j), jobs):
            if r:
                rows += r if isinstance(r, list) else [r]

    os.makedirs(args.outdir, exist_ok=True)
    with open(os.path.join(args.outdir, "results.tsv"), "w") as f:
        for d, name, exp, got, dt, v, first in rows:
            exp_s = " ".join(f"{k}={'T' if x else 'F'}" for k, x in sorted(exp.items()))
            f.write(f"{d}\t{name}\t{exp_s}\t{got}\t{dt:.1f}\t{v}\t{first}\n")

    per = {}
    for d, *_, v, _first in rows:
        per.setdefault(d, {}).setdefault(v, 0)
        per[d][v] += 1
    total = {}
    for d in sorted(per):
        print(f"{d:26} " + "  ".join(f"{k}={n}" for k, n in sorted(per[d].items())))
        for k, n in per[d].items():
            total[k] = total.get(k, 0) + n
    print(f"{'TOTAL':26} " + "  ".join(f"{k}={n}" for k, n in sorted(total.items())))


if __name__ == "__main__":
    main()
