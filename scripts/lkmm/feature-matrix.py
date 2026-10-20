#!/usr/bin/env python3
"""Feature matrix for the -lkmm tests (lkmm-implementation plan, 5.6).

Runs every tests/correct/lkmm test (each args line, each variant) under
-lkmm with a set of option variations, and every wrong test that opts into
-lkmm (genmc.lkmm.*.in) with -print-error-trace and -dump-error-graph.
A test's matrix-skip file can leave configurations out (with the issue
that makes them differ). Correct runs must give the expected count (estimation must give an
estimate; -disable-spin-assume must give no error; -check-liveness may
report a liveness violation if the base run has blocked executions), wrong
runs must report the base run's error.

  scripts/lkmm/feature-matrix.py [--genmc build/bin/genmc] [--jobs 8]
	[--json out.jsonl] [--dots DIR]
"""
import argparse, glob, json, os, re, subprocess, sys, tempfile
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", ".."))
CONFIGS = {
    "base": [], "nthreads4": ["-nthreads=4"],
    "arb1": ["-schedule-policy=arbitrary", "-schedule-seed=1"],
    "arb2": ["-schedule-policy=arbitrary", "-schedule-seed=2"],
    "arb3": ["-schedule-policy=arbitrary", "-schedule-seed=3"],
    "ltr": ["-schedule-policy=ltr"], "wfr": ["-schedule-policy=wfr"],
    "nospin": ["-disable-spin-assume"], "liveness": ["-check-liveness"],
    "estimate": ["-mode=estimate"],
}


def run(genmc, args, src, timeout):
    cmd = [genmc, "-lkmm", "-disable-mm-detector"] + args + \
        ["--", f"-I{ROOT}/lli/runtime-include/c"] + src
    try:
        p = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout)
    except subprocess.TimeoutExpired:
        return {"rc": "timeout"}
    out = p.stdout + p.stderr
    num = lambda pat: (lambda m: int(m.group(1)) if m else None)(re.search(pat, out))
    err = re.search(r"^Error(?: detected)?: (.*)$", out, re.M)
    return {"rc": p.returncode, "n": num(r"complete executions explored: (\d+)"),
            "blocked": num(r"blocked executions seen: (\d+)") or 0,
            "est": num(r"Total executions estimate: (\d+)"),
            "err": err.group(1) if err else None,
            "tail": out.strip().splitlines()[-3:]}


def matrix_skip(d):
    """The configurations a test's matrix-skip file leaves out: one name per
    line, then a reason after '#' (a known issue, e.g. a count that depends
    on the exploration order)"""
    f = d + "matrix-skip"
    if not os.path.exists(f):
        return set()
    return {l.split("#")[0].strip() for l in open(f)} - {""}


def jobs(dots):
    for d in sorted(glob.glob(f"{ROOT}/tests/correct/lkmm/*/")):
        a, x = d + "args.lkmm.mo.in", d + "expected.lkmm.mo.in"
        if not os.path.exists(x):
            continue
        lines = open(a).read().splitlines() if os.path.exists(a) else ["|"]
        skip = matrix_skip(d)
        for i, (line, exp) in enumerate(zip(lines, open(x).read().split())):
            gargs, _, cargs = line.partition("|")
            for v in sorted(glob.glob(d + "variants/*.c")):
                for c, opts in CONFIGS.items():
                    if c in skip:
                        continue
                    yield ("correct", d, i, v, c, gargs.split() + opts, cargs.split() + [v], int(exp))
    for f in sorted(glob.glob(f"{ROOT}/tests/wrong/*/*/genmc.lkmm.*.in")):
        d = os.path.dirname(f) + "/"
        for v in sorted(glob.glob(d + "variants/*.c")):
            dot = os.path.join(dots, f"{os.path.basename(d[:-1])}-{os.path.basename(v)}")
            for c, opts in CONFIGS.items():
                if c != "estimate":
                    extra = ["-print-error-trace", f"-dump-error-graph={dot}.{c}.dot"]
                    yield ("wrong", d, 0, v, c, open(f).read().split() + opts + extra, [v], None)


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--genmc", default=f"{ROOT}/build/bin/genmc")
    ap.add_argument("--jobs", type=int, default=8)
    ap.add_argument("--timeout", type=int, default=300)
    ap.add_argument("--json")
    ap.add_argument("--dots")
    o = ap.parse_args()
    dots = o.dots or tempfile.mkdtemp(prefix="lkmm-matrix-")
    os.makedirs(dots, exist_ok=True)

    def go(j):
        kind, d, i, v, c, gargs, src, exp = j
        return {"kind": kind, "test": os.path.basename(d[:-1]), "line": i,
                "variant": os.path.basename(v), "config": c, "exp": exp,
                **run(o.genmc, gargs, src, o.timeout)}

    with ThreadPoolExecutor(o.jobs) as ex:
        rows = list(ex.map(go, list(jobs(dots))))
    if o.json:
        with open(o.json, "w") as f:
            for r in rows:
                f.write(json.dumps(r) + "\n")

    key = lambda r: (r["kind"], r["test"], r["line"], r["variant"])
    base = {key(r): r for r in rows if r["config"] == "base"}
    bad = []
    for r in rows:
        c, b = r["config"], base[key(r)]
        if r["rc"] == "timeout":
            bad.append((r, "timeout"))
        elif r["kind"] == "wrong":
            if r["rc"] != 42 or r["err"] != b["err"]:
                bad.append((r, f"rc={r['rc']} error={r['err']!r}, base {b['err']!r}"))
        elif c == "estimate":
            if r["rc"] != 0 or r["est"] is None:
                bad.append((r, f"rc={r['rc']} no estimate {r['tail']}"))
        elif c == "liveness" and r["rc"] == 42 and b["blocked"] and \
                (r["err"] or "").startswith("Liveness violation"):
            pass  # a thread blocked for good (e.g. exits holding a lock)
        elif c == "nospin":
            if r["rc"] != 0 or r["err"]:
                bad.append((r, f"rc={r['rc']} error={r['err']!r}"))
        elif r["rc"] != 0 or r["err"] or r["n"] != r["exp"]:
            bad.append((r, f"rc={r['rc']} n={r['n']} expected {r['exp']} {r['err'] or ''} {r['tail']}"))
    for r, why in bad:
        print(f"{r['config']:10} {r['kind']:7} {r['test']}/{r['variant']} (line {r['line'] + 1}): {why}")
    print(f"{len(rows)} runs, {len(bad)} deviations; error graphs in {dots}")
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
