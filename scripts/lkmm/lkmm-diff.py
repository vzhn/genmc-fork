#!/usr/bin/env python3
"""Differential test of GenMC against herd7 on C litmus tests.

For each test, compares the set of final states GenMC explores with the set
herd7 reports (its "States" list, the projection of every allowed execution
onto the condition's observables), and GenMC's race report with herd7's
"Flag data-race".

herd7's outputs come from run-herd.sh: <herd-out>/<path relative to
--litmus-root>.out. GenMC runs once on the litmus2c.py translation, with
-print-exec-graphs: a race stops it (and only the race verdicts are
compared); otherwise it has printed every execution's final state.

Each test gets one result:
  agree                  same states (and same race verdict)
  agree-racy             both report a data race (a racy program's states are
                         undefined in C11 and LKMM alike, so not compared)
  missing                GenMC lacks some of herd7's states (stronger)
  extra                  GenMC has states herd7 forbids (weaker)
  differ                 both missing and extra states
  race                   same states, race verdicts differ
  count                  same states and races, but GenMC explores a different
                         number of executions than herd7's Positive+Negative
                         (the two count the same thing: consistent rf/co
                         choices); a disagreement only with --strict-counts
  skipped:<reason>       translator does not support the test
  herd-timeout, genmc-timeout, genmc-error:<msg>, herd-error
A result listed in the allow-list (lines "<test> <result> <reason>",
test paths relative to --litmus-root) is reported as allowed. Exit status is
1 if any other result is a disagreement (missing/extra/differ/race/genmc-error).

Usage: lkmm-diff.py --herd-out DIR --litmus-root DIR [options] <file-or-dir>...
"""

import argparse
import collections
import concurrent.futures
import json
import os
import re
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.dont_write_bytecode = True  # no __pycache__ in the source tree
import litmus2c  # noqa: E402

CLANG_FLAGS = ["-Wno-int-conversion", "-Wno-incompatible-pointer-types",
               "-Wno-int-to-pointer-cast", "-Wno-pointer-to-int-cast"]
DISAGREE = ("missing", "extra", "differ", "race", "genmc-error")


def herd_result(path):
    """(states, flags, status) from a herd7 output file"""
    if not os.path.exists(path):
        return None, set(), "herd-missing"
    text = open(path, errors="replace").read()
    if re.search(r"^TIMEOUT", text, re.M):
        return None, set(), "herd-timeout"
    m = re.search(r"^States (\d+)\n", text, re.M)
    if not m:
        return None, set(), "herd-error"
    n = int(m.group(1))
    lines = text[m.end():].split("\n")[:n]
    states = set()
    for line in lines:
        st = {}
        for item in line.split(";"):
            item = item.strip()
            if not item:
                continue
            k, _, v = item.partition("=")
            # herd7 writes a memory location x as [x]
            st[k.strip().strip("[]")] = v.strip()
        states.add(frozenset(st.items()))
    flags = set(re.findall(r"^Flag ([\w-]+)", text, re.M))
    m = re.search(r"^Positive: (\d+) Negative: (\d+)", text, re.M)
    if m:
        flags.add("execs=%d" % (int(m.group(1)) + int(m.group(2))))
    return states, flags, "ok"


def genmc_states(out, meta):
    """The final states in -print-exec-graphs output"""
    obs = meta["observables"]
    addrs = meta["addresses"]
    mark = "OUTPUT: " + meta["output_mark"]
    states = set()
    for block in re.split(r"\n(?=<-1, 0>:)", out):
        if mark not in block:
            continue
        main = block.split("\n<", 1)[0]  # thread 0 comes first
        after = main.split(mark, 1)[1]
        vals = re.findall(r"^\s*\(0, \d+\): W\w* \([^)]*, (-?\d+)\)", after, re.M)
        if len(vals) != len(obs):
            raise RuntimeError("expected %d observable writes, found %d" % (len(obs), len(vals)))
        # values are printed unsigned; sentinels for addresses are negative
        vals = [str(int(v) - 2**64 if int(v) >= 2**63 else int(v)) for v in vals]
        st = {o: addrs.get(v, v) for o, v in zip(obs, vals)}
        states.add(frozenset(st.items()))
    return states


def run_genmc(args, src, extra):
    cmd = [args.genmc, "-" + args.model, "-disable-estimation", "-disable-mm-detector"]
    cmd += args.genmc_flags.split() + extra + ["--"] + CLANG_FLAGS + [src]
    try:
        p = subprocess.run(cmd, capture_output=True, text=True, timeout=args.timeout)
    except subprocess.TimeoutExpired:
        return None, "genmc-timeout"
    return p.stdout + p.stderr, p.returncode


def check(args, path):
    rel = os.path.relpath(os.path.realpath(path), args.litmus_root)
    res = {"test": rel}
    hstates, hflags, hstatus = herd_result(
        os.path.join(args.herd_out, os.path.splitext(rel)[0] + ".out"))
    hexecs = [f for f in hflags if f.startswith("execs=")]
    hflags -= set(hexecs)
    res["herd_execs"] = int(hexecs[0][6:]) if hexecs else None
    res["herd_flags"] = sorted(hflags)
    try:
        lt = litmus2c.Litmus(open(path, errors="replace").read())
        src, meta = litmus2c.Translator(lt, args.header).translate()
    except litmus2c.Unsupported as e:
        res["result"] = "skipped:" + str(e)
        return res
    res["expected"] = meta["expected"]
    if hstatus != "ok":
        res["result"] = hstatus
        return res
    with tempfile.TemporaryDirectory(dir=args.tmp) as d:
        c = os.path.join(d, "t.c")
        open(c, "w").write(src)
        out, rc = run_genmc(args, c, ["-print-exec-graphs"])
        if out is None:
            res["result"] = rc
            return res
        grace = re.search(r"^Error: Non-atomic race", out, re.M) is not None
        err = re.search(r"^(Error[^\n]*|[A-Z][\w-]* violation[^\n]*)", out, re.M)
        if not grace and (rc != 0 or err):
            msg = err.group(1) if err else out.strip().split("\n")[-1]
            res["result"] = "genmc-error:" + msg[:80]
            return res
        try:
            gstates = set() if grace else genmc_states(out, meta)
        except RuntimeError as e:
            res["result"] = "genmc-error:" + str(e)
            return res
        m = re.search(r"complete executions explored: (\d+)", out)
        res["genmc_execs"] = int(m.group(1)) if m else None
    res["herd_states"] = len(hstates)
    res["genmc_states"] = len(gstates)
    missing, extra = hstates - gstates, gstates - hstates
    res["missing"] = [dict(s) for s in sorted(missing, key=sorted)][:5]
    res["extra"] = [dict(s) for s in sorted(extra, key=sorted)][:5]
    if grace or "data-race" in hflags:
        res["result"] = "agree-racy" if grace and "data-race" in hflags else "race"
        res["genmc_race"] = grace
        res["missing"], res["extra"] = [], []
    elif missing and extra:
        res["result"] = "differ"
    elif missing:
        res["result"] = "missing"
    elif extra:
        res["result"] = "extra"
    elif res["herd_execs"] is not None and res["genmc_execs"] != res["herd_execs"]:
        res["result"] = "count"
    else:
        res["result"] = "agree"
    return res


def load_allowlist(path):
    allow = {}
    if not path or not os.path.exists(path):
        return allow
    for line in open(path):
        line = line.split("#", 1)[0].strip()
        if not line:
            continue
        parts = line.split(None, 2)
        allow[parts[0]] = (parts[1], parts[2] if len(parts) > 2 else "")
    return allow


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("paths", nargs="+")
    ap.add_argument("--herd-out", required=True)
    ap.add_argument("--litmus-root", required=True)
    ap.add_argument("--genmc", default=os.path.join(HERE, "../../build/bin/genmc"))
    ap.add_argument("--model", default="imm")
    ap.add_argument("--genmc-flags", default="")
    ap.add_argument("--header", default=os.path.join(HERE, "lkmm-c11.h"))
    ap.add_argument("--timeout", type=int, default=60)
    ap.add_argument("--jobs", type=int, default=os.cpu_count())
    ap.add_argument("--allowlist")
    ap.add_argument("--json", help="write one JSON result per line here")
    ap.add_argument("--tmp", default=None)
    ap.add_argument("--strict-counts", action="store_true",
                    help="treat a different number of executions as a disagreement")
    args = ap.parse_args()
    args.litmus_root = os.path.realpath(args.litmus_root)
    args.genmc = os.path.realpath(args.genmc)
    args.header = os.path.realpath(args.header)

    files = []
    for p in args.paths:
        if os.path.isdir(p):
            for d, _, fs in os.walk(p):
                files += [os.path.join(d, f) for f in fs if f.endswith(".litmus")]
        else:
            files.append(p)
    files.sort()

    allow = load_allowlist(args.allowlist)
    results = []
    with concurrent.futures.ThreadPoolExecutor(args.jobs) as ex:
        for res in ex.map(lambda f: check(args, f), files):
            a = allow.get(res["test"])
            if a and a[0] == res["result"].split(":")[0]:
                res["allowed"] = a[1]
            results.append(res)
            print("%-14s %s" % (res["result"].split(":")[0] + ("*" if "allowed" in res else ""),
                                res["test"]), flush=True)
    if args.json:
        with open(args.json, "w") as f:
            for r in results:
                f.write(json.dumps(r) + "\n")

    # summary: result kind per top-level directory
    table = collections.defaultdict(collections.Counter)
    for r in results:
        top = r["test"].split(os.sep)[0] if os.sep in r["test"] else "."
        kind = r["result"].split(":")[0] + ("*" if "allowed" in r else "")
        table[top][kind] += 1
        table["total"][kind] += 1
    kinds = sorted({k for c in table.values() for k in c})
    print("\n| dir | " + " | ".join(kinds) + " |")
    print("|---|" + "---|" * len(kinds))
    for top in sorted(table, key=lambda t: (t == "total", t)):
        print("| %s | %s |" % (top, " | ".join(str(table[top][k]) for k in kinds)))
    skipped = collections.Counter(r["result"][8:] for r in results
                                  if r["result"].startswith("skipped:"))
    if skipped:
        print("\nskipped: " + ", ".join("%s (%d)" % kv for kv in skipped.most_common(12)))
    disagree = DISAGREE + (("count",) if args.strict_counts else ())
    bad = [r for r in results if r["result"].split(":")[0] in disagree and "allowed" not in r]
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
