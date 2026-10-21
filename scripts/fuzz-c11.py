#!/usr/bin/env python3
"""Differential fuzzer for GenMC's exploration: random C11 programs, run in
the default mode, under arbitrary schedules, without resume points and with
-exhaustive (the reference exploration), compared on the number of complete
executions and on the verdict.

    scripts/fuzz-c11.py gen <seed> [-o file.c]
    scripts/fuzz-c11.py run --genmc BIN [--models rc11,imm] [--seeds 0-499]
                            [--jobs N] [--json out.jsonl] [--workdir DIR]
                            [--timeout S] [--cap N] [--schedules K]

Seeds are stable: `gen <seed>` is the same program on every run. A program
has 2 to 4 worker threads, 2 to 4 shared atomic_int globals and 2 to 6
operations per thread: loads, stores, fetch_add, strong and weak
compare_exchange, fences, with memory orders drawn from relaxed, acquire,
release, acq_rel and seq_cst, `if` branches on loaded values, and sometimes
a plain (non-atomic) variable written by one thread before a release store
of a flag that only it writes, and read by the others only after an acquire
load of the flag saw the write (no data race).

`run` runs, per seed and model (with -disable-estimation -disable-mm-detector):
  default                 the default exploration
  sched-<k>               the default exploration under an arbitrary schedule
                          (-schedule-policy=arbitrary -schedule-seed=N), K times
  no-resume               -resume-points=0 (forward revisits replay the graph)
  exhaustive              -exhaustive, the reference
  exhaustive-sched        -exhaustive under an arbitrary schedule (the
                          reference must not depend on the schedule either)
and reports every seed whose runs disagree on the number of complete executions
or on the verdict (no errors, or the error reported), with the seed, the model,
the program's path and each run's result; then a summary line. A seed whose
default run explores more than --cap executions (default 5000) or times out
(--timeout, default 120 s) is skipped. The exit status is 1 if any seed
disagreed. --json writes one JSON object per seed and model.

Every disagreement is a bug: in the reference (fix it) or in the exploration
(an issue and a regression test). No third-party packages are needed.
"""

import argparse
import json
import os
import random
import re
import subprocess
import sys
import time
from concurrent.futures import ThreadPoolExecutor

LOAD_ORDERS = ["memory_order_relaxed", "memory_order_acquire", "memory_order_seq_cst"]
STORE_ORDERS = ["memory_order_relaxed", "memory_order_release", "memory_order_seq_cst"]
RMW_ORDERS = ["memory_order_relaxed", "memory_order_acquire", "memory_order_release",
              "memory_order_acq_rel", "memory_order_seq_cst"]
FENCE_ORDERS = ["memory_order_acquire", "memory_order_release", "memory_order_acq_rel",
                "memory_order_seq_cst"]


class Gen:
    """One random program (deterministic in the seed)"""

    def __init__(self, seed):
        self.rnd = random.Random(seed)
        self.seed = seed
        self.nthreads = self.rnd.choice([2, 2, 3, 3, 4])
        self.nvars = self.rnd.choice([2, 2, 3, 3, 4])
        self.vars = ["x%d" % i for i in range(self.nvars)]
        # A plain variable published through a flag that one thread alone
        # writes (readers check the flag with an acquire or seq_cst load)
        self.plain = self.rnd.random() < 0.3
        self.plain_writer = self.rnd.randrange(self.nthreads) if self.plain else -1
        self.nlocals = [0] * self.nthreads

    def local(self, t):
        name = "r%d" % self.nlocals[t]
        self.nlocals[t] += 1
        return name

    def const(self):
        return self.rnd.choice([0, 1, 1, 2, 2, 3])

    def var(self):
        return self.rnd.choice(self.vars)

    def op(self, t, loaded, depth, budget):
        """One statement (a list of lines); appends loaded locals to LOADED.
        Returns the lines and the number of operations they contain."""
        k = self.rnd.random()
        if k < 0.22:
            r = self.local(t)
            loaded.append(r)
            return (["%s = atomic_load_explicit(&%s, %s);" %
                     (r, self.var(), self.rnd.choice(LOAD_ORDERS))], 1)
        if k < 0.46:
            return (["atomic_store_explicit(&%s, %d, %s);" %
                     (self.var(), self.const(), self.rnd.choice(STORE_ORDERS))], 1)
        if k < 0.58:
            r = self.local(t)
            loaded.append(r)
            return (["%s = atomic_fetch_add_explicit(&%s, %d, %s);" %
                     (r, self.var(), self.rnd.choice([1, 1, 2]), self.rnd.choice(RMW_ORDERS))],
                    1)
        if k < 0.72:
            r = self.local(t)
            e = self.local(t)
            loaded.append(e)
            strong = self.rnd.random() < 0.7
            succ = self.rnd.choice(RMW_ORDERS)
            # The failure order may not be stronger than the success order,
            # nor release/acq_rel
            fails = ["memory_order_relaxed"]
            if succ in ("memory_order_acquire", "memory_order_acq_rel", "memory_order_seq_cst"):
                fails.append("memory_order_acquire")
            if succ == "memory_order_seq_cst":
                fails.append("memory_order_seq_cst")
            fail = self.rnd.choice(fails)
            return (["%s = %d;" % (e, self.const()),
                     "%s = atomic_compare_exchange_%s_explicit(&%s, &%s, %d, %s, %s);" %
                     (r, "strong" if strong else "weak", self.var(), e, self.const(), succ,
                      fail)], 1)
        if k < 0.80:
            return (["atomic_thread_fence(%s);" % self.rnd.choice(FENCE_ORDERS)], 1)
        if k < 0.92 and depth < 2 and loaded and budget >= 2:
            r = self.rnd.choice(loaded)
            cond = self.rnd.choice(["%s == %d" % (r, self.const()), "%s" % r, "%s != %d" %
                                    (r, self.const())])
            body, n = self.block(t, loaded, depth + 1, min(budget - 1, self.rnd.randint(1, 2)))
            out = ["if (%s) {" % cond] + ["\t" + l for l in body]
            if self.rnd.random() < 0.4 and budget - n >= 2:
                els, m = self.block(t, loaded, depth + 1, min(budget - 1 - n, 2))
                n += m
                out += ["} else {"] + ["\t" + l for l in els]
            out.append("}")
            return (out, n)
        return (["atomic_store_explicit(&%s, %d, %s);" %
                 (self.var(), self.const(), self.rnd.choice(STORE_ORDERS))], 1)

    def block(self, t, loaded, depth, budget):
        lines = []
        n = 0
        while n < budget:
            l, m = self.op(t, loaded, depth, budget - n)
            lines += l
            n += m
        return lines, n

    def thread(self, t):
        loaded = []
        budget = self.rnd.randint(2, 6)
        lines = []
        if self.plain and t == self.plain_writer:
            # The writer publishes the plain variable first (and may use it)
            lines += ["p = %d;" % self.rnd.choice([1, 2, 3]),
                      "atomic_store_explicit(&f, 1, %s);" %
                      self.rnd.choice(["memory_order_release", "memory_order_seq_cst"])]
        body, n = self.block(t, loaded, 0, budget)
        lines += body
        if self.plain and t != self.plain_writer and self.rnd.random() < 0.8:
            r = self.local(t)
            q = self.local(t)
            lines += ["%s = atomic_load_explicit(&f, %s);" %
                      (r, self.rnd.choice(["memory_order_acquire", "memory_order_seq_cst"])),
                      "if (%s == 1)" % r, "\t%s = p;" % q]
        decls = ["int %s = 0;" % ("r%d" % i) for i in range(self.nlocals[t])]
        return decls + lines

    def program(self):
        threads = [self.thread(t) for t in range(self.nthreads)]
        out = ["/* Generated by scripts/fuzz-c11.py, seed %d */" % self.seed,
               "#include <pthread.h>", "#include <stdatomic.h>", ""]
        out += ["atomic_int %s;" % v for v in self.vars]
        if self.plain:
            out += ["atomic_int f;", "int p;"]
        out.append("")
        for t, lines in enumerate(threads):
            out.append("void *t%d(void *arg)" % t)
            out.append("{")
            out += ["\t" + l for l in lines]
            out += ["\treturn NULL;", "}", ""]
        out += ["int main(void)", "{", "\tpthread_t %s;" %
                ", ".join("h%d" % t for t in range(self.nthreads))]
        out += ["\tpthread_create(&h%d, NULL, t%d, NULL);" % (t, t) for t in range(self.nthreads)]
        out += ["\tpthread_join(h%d, NULL);" % t for t in range(self.nthreads)]
        out += ["\treturn 0;", "}", ""]
        return "\n".join(out)


def generate(seed):
    return Gen(seed).program()


# ---------------------------------------------------------------------------

RE_EXPLORED = re.compile(r"Number of complete executions explored: (\d+)")
RE_RAW = re.compile(r"^Exhaustive: (\d+) complete executions reached", re.M)


def verdict(output):
    """"none" without errors, else the first error or warning line"""
    if "No errors were detected" in output:
        return "none"
    for line in output.splitlines():
        if line.startswith("Error:") or line.startswith("Warning:"):
            return " ".join(line.split())[:60]
    return "none"


def run_genmc(genmc, model, flags, path, timeout, extra):
    cmd = [genmc, "-" + model, "-disable-estimation", "-disable-mm-detector"] + flags + extra + \
          ["--", path]
    t0 = time.time()
    try:
        p = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                           timeout=timeout, text=True, errors="replace")
    except subprocess.TimeoutExpired:
        return {"status": "timeout", "time": time.time() - t0}
    out = p.stdout
    m = RE_EXPLORED.search(out)
    v = verdict(out)
    res = {"status": "ok" if (m or v != "none") else "fail", "time": round(time.time() - t0, 2),
           "count": int(m.group(1)) if m else None, "verdict": v, "rc": p.returncode}
    r = RE_RAW.search(out)
    if r:
        res["raw"] = int(r.group(1))
    if res["status"] == "fail":
        res["tail"] = out.strip().splitlines()[-3:]
    return res


def configs(seed, schedules):
    cs = [("default", [])]
    for k in range(schedules):
        cs.append(("sched-%d" % k,
                   ["-schedule-policy=arbitrary", "-schedule-seed=%d" % (seed * 7919 + k + 1)]))
    cs.append(("no-resume", ["-resume-points=0"]))
    cs.append(("exhaustive", ["-exhaustive"]))
    cs.append(("exhaustive-sched", ["-exhaustive", "-schedule-policy=arbitrary",
                                    "-schedule-seed=%d" % (seed * 7919 + 1)]))
    return cs


def fuzz_one(args, seed, model):
    path = os.path.join(args.workdir, "fuzz-%d.c" % seed)
    if not os.path.exists(path):
        with open(path, "w") as f:
            f.write(generate(seed))
    rec = {"seed": seed, "model": model, "file": path, "runs": {}}
    cs = configs(seed, args.schedules)
    name, flags = cs[0]
    r = run_genmc(args.genmc, model, flags, path, args.timeout, args.extra)
    rec["runs"][name] = r
    if r["status"] == "timeout":
        rec["status"] = "skip-timeout"
        return rec
    if r["status"] == "fail":
        rec["status"] = "fail"
        return rec
    if r["count"] is not None and r["count"] > args.cap:
        rec["status"] = "skip-cap"
        return rec
    for name, flags in cs[1:]:
        rec["runs"][name] = run_genmc(args.genmc, model, flags, path, args.timeout, args.extra)
    ref = (r["count"], r["verdict"])
    status = "agree"
    for name, rr in rec["runs"].items():
        if rr["status"] == "timeout":
            status = "timeout" if status == "agree" else status
        elif rr["status"] == "fail":
            status = "fail"
        elif (rr["count"], rr["verdict"]) != ref:
            status = "disagree"
    rec["status"] = status
    return rec


def parse_seeds(spec):
    seeds = []
    for part in spec.split(","):
        if "-" in part:
            a, b = part.split("-")
            seeds += list(range(int(a), int(b) + 1))
        else:
            seeds.append(int(part))
    return seeds


def fmt_run(r):
    if r["status"] == "timeout":
        return "timeout"
    if r["status"] == "fail":
        return "fail(%s)" % "; ".join(r.get("tail", []))[:80]
    s = "%s %s" % (r["count"], r["verdict"])
    if "raw" in r:
        s += " (raw %d)" % r["raw"]
    return s


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    sub = ap.add_subparsers(dest="cmd", required=True)
    g = sub.add_parser("gen", help="print or write the program of a seed")
    g.add_argument("seed", type=int)
    g.add_argument("-o", "--output")
    r = sub.add_parser("run", help="run the differential comparison over seeds")
    r.add_argument("--genmc", required=True)
    r.add_argument("--models", default="rc11,imm")
    r.add_argument("--seeds", default="0-99")
    r.add_argument("--jobs", type=int, default=1)
    r.add_argument("--json")
    r.add_argument("--workdir", default="fuzz-c11")
    r.add_argument("--timeout", type=float, default=120)
    r.add_argument("--cap", type=int, default=5000)
    r.add_argument("--schedules", type=int, default=2)
    r.add_argument("--extra", nargs="*", default=[], help="extra GenMC flags for every run")
    r.add_argument("-v", "--verbose", action="store_true", help="print agreeing seeds too")
    args = ap.parse_args()

    if args.cmd == "gen":
        src = generate(args.seed)
        if args.output:
            with open(args.output, "w") as f:
                f.write(src)
        else:
            sys.stdout.write(src)
        return 0

    args.genmc = os.path.abspath(args.genmc)
    os.makedirs(args.workdir, exist_ok=True)
    seeds = parse_seeds(args.seeds)
    models = args.models.split(",")
    jobs = [(s, m) for s in seeds for m in models]
    stats = {}
    disagreements = []
    jf = open(args.json, "w") if args.json else None
    with ThreadPoolExecutor(max_workers=args.jobs) as ex:
        for rec in ex.map(lambda sm: fuzz_one(args, sm[0], sm[1]), jobs):
            stats[rec["status"]] = stats.get(rec["status"], 0) + 1
            if jf:
                jf.write(json.dumps(rec) + "\n")
                jf.flush()
            if rec["status"] in ("disagree", "fail", "timeout"):
                disagreements.append(rec)
                print("%s seed %d %s %s" % (rec["status"].upper(), rec["seed"], rec["model"],
                                            rec["file"]))
                for name, rr in rec["runs"].items():
                    print("    %-10s %s" % (name, fmt_run(rr)))
            elif args.verbose:
                print("%s seed %d %s: %s" % (rec["status"], rec["seed"], rec["model"],
                                             fmt_run(rec["runs"]["default"])))
            sys.stdout.flush()
    if jf:
        jf.close()
    total = len(jobs)
    print("fuzz-c11 summary: %d runs (%d seeds x %s): %s" %
          (total, len(seeds), ",".join(models),
           ", ".join("%d %s" % (n, k) for k, n in sorted(stats.items()))))
    return 1 if any(d["status"] == "disagree" for d in disagreements) else 0


if __name__ == "__main__":
    sys.exit(main())
