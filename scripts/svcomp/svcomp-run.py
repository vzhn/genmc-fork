#!/usr/bin/env python3
#
# This program is dual-licensed under the Apache License 2.0 and the MIT License.
# You may choose to use, distribute, or modify this software under either license.

"""Run GenMC on SV-COMP tasks and compare its result with the expected verdicts.

Usage: svcomp-run.py [-j JOBS] [-t TIMEOUT] [-m MODEL] [-o OUTDIR]
                     [--nondet-max N] [--max-choices N] [--max-spurious N]
                     GENMC TASK_DIR...

For each <task>.yml, runs GenMC on the task's .c source with svcomp-shim.h and
maps the outcome to the properties GenMC can decide:
  unreach-call    false if GenMC reports a safety violation (failed assertion)
  no-data-race    false if GenMC reports a non-atomic race
  valid-memsafety false if GenMC reports a memory error
GenMC stops at the first error, so a task whose verdict is false for one
property is only checked for that property.

A race where one side lies inside an atomic section is labelled ATOMIC-RACE
and gets no verdict: SV-COMP counts accesses in atomic sections as atomic,
while the shim implements atomic sections with a lock.

Tasks that include <pthread.h> get svcomp-shim.h. Preprocessed tasks, which
declare the pthread API themselves, get svcomp-shim-pp.h and a copy of the task
with a generated tail that defines the pthread functions the task mentions,
using the task's own types.

Writes OUTDIR/results.tsv and one OUTDIR/<dir>/<task>.out per task.
"""
import argparse
import concurrent.futures as cf
import os
import re
import subprocess
import time

import yaml

PROPS = {"unreach-call", "no-data-race", "valid-memsafety"}
HERE = os.path.dirname(os.path.abspath(__file__))
SHIM = os.path.join(HERE, "svcomp-shim.h")
SHIM_PP = os.path.join(HERE, "svcomp-shim-pp.h")

# Definitions for preprocessed tasks, in terms of GenMC's primitives. Each is
# emitted only if the task mentions the function, so the types it uses exist.
TAIL = {
    "pthread_create": """int pthread_create(pthread_t *t, const pthread_attr_t *a,
		   void *(*f)(void *), void *arg)
{ *t = __VERIFIER_thread_create(0, f, arg); return 0; }""",
    "pthread_join": """int pthread_join(pthread_t t, void **r)
{ void *v = __VERIFIER_thread_join(t); if (r) *r = v; return 0; }""",
    "pthread_exit": "void pthread_exit(void *r) { __VERIFIER_thread_exit(r); }",
    "pthread_self": "pthread_t pthread_self(void) { return __VERIFIER_thread_self(); }",
    "pthread_mutex_init": """int pthread_mutex_init(pthread_mutex_t *m, const pthread_mutexattr_t *a)
{ return __VERIFIER_mutex_init((__VERIFIER_mutex_t *) m, 0); }""",
    "pthread_mutex_destroy": """int pthread_mutex_destroy(pthread_mutex_t *m)
{ return __VERIFIER_mutex_destroy((__VERIFIER_mutex_t *) m); }""",
    "pthread_mutex_lock": """int pthread_mutex_lock(pthread_mutex_t *m)
{ return __VERIFIER_mutex_lock((__VERIFIER_mutex_t *) m); }""",
    "pthread_mutex_trylock": """int pthread_mutex_trylock(pthread_mutex_t *m)
{ return __VERIFIER_mutex_trylock((__VERIFIER_mutex_t *) m); }""",
    "pthread_mutex_unlock": """int pthread_mutex_unlock(pthread_mutex_t *m)
{ return __VERIFIER_mutex_unlock((__VERIFIER_mutex_t *) m); }""",
    "pthread_cond_init": """int pthread_cond_init(pthread_cond_t *c, const pthread_condattr_t *a)
{ __atomic_store_n((int *) c, 0, __ATOMIC_RELAXED); return 0; }""",
    "pthread_cond_destroy": "int pthread_cond_destroy(pthread_cond_t *c) { return 0; }",
    "pthread_cond_wait": """int pthread_cond_wait(pthread_cond_t *c, pthread_mutex_t *m)
{
	int seq = __atomic_load_n((int *) c, __ATOMIC_ACQUIRE);
	int spurious = __svc_spurious_wakeup();
	__VERIFIER_mutex_unlock((__VERIFIER_mutex_t *) m);
	if (!spurious)
		__VERIFIER_await(__atomic_load_n((int *) c, __ATOMIC_ACQUIRE) != seq);
	return __VERIFIER_mutex_lock((__VERIFIER_mutex_t *) m);
}""",
    "pthread_cond_signal": """int pthread_cond_signal(pthread_cond_t *c)
{ __atomic_fetch_add((int *) c, 1, __ATOMIC_RELEASE); return 0; }""",
    "pthread_cond_broadcast": """int pthread_cond_broadcast(pthread_cond_t *c)
{ __atomic_fetch_add((int *) c, 1, __ATOMIC_RELEASE); return 0; }""",
    "pthread_once": """int pthread_once(pthread_once_t *o, void (*fn)(void))
{
	int s = 0;
	if (__atomic_compare_exchange_n((int *) o, &s, 1, 0, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE)) {
		fn();
		__atomic_store_n((int *) o, 2, __ATOMIC_RELEASE);
		return 0;
	}
	__VERIFIER_await(__atomic_load_n((int *) o, __ATOMIC_ACQUIRE) == 2);
	return 0;
}""",
    "pthread_attr_init": "int pthread_attr_init(pthread_attr_t *a) { return 0; }",
    "pthread_attr_destroy": "int pthread_attr_destroy(pthread_attr_t *a) { return 0; }",
    "pthread_attr_setdetachstate": "int pthread_attr_setdetachstate(pthread_attr_t *a, int s) { return 0; }",
    "pthread_mutexattr_init": "int pthread_mutexattr_init(pthread_mutexattr_t *a) { return 0; }",
    "pthread_mutexattr_destroy": "int pthread_mutexattr_destroy(pthread_mutexattr_t *a) { return 0; }",
    "pthread_mutexattr_settype": "int pthread_mutexattr_settype(pthread_mutexattr_t *a, int k) { return 0; }",
    # GenMC's allocator rejects size 0 (its stdlib.h asserts); malloc(0) is
    # legal C, so drop those executions instead of reporting an error
    "malloc": "void *malloc(size_t n) { __VERIFIER_assume(n > 0); return __VERIFIER_malloc(n); }",
    "calloc": """void *calloc(size_t n, size_t sz)
{ __VERIFIER_assume(n > 0 && sz > 0); return __VERIFIER_calloc(n, sz); }""",
    "free": "void free(void *p) { __VERIFIER_free(p); }",
    # SV-COMP's abort() cuts off a path; GenMC's real abort() would kill GenMC
    "abort": "void abort(void) { __VERIFIER_assume(0); }",
    "pthread_detach": "int pthread_detach(pthread_t t) { return 0; }",
    "__assert_fail": """void __assert_fail(const char *e, const char *file, unsigned int line,
		   const char *fn)
{ __VERIFIER_assert_fail(e, file, line); }""",
}


def with_local_includes(code, d, seen=None):
    """CODE followed by the headers it includes with quotes from directory D
    (recursively): some tasks define main in such a header."""
    seen = set() if seen is None else seen
    out = [code]
    for h in re.findall(r'^\s*#\s*include\s*"([^"]+)"', code, re.M):
        path = os.path.normpath(os.path.join(d, h))
        if path in seen or not os.path.isfile(path):
            continue
        seen.add(path)
        out.append(with_local_includes(open(path, errors="replace").read(),
                                       os.path.dirname(path), seen))
    return "\n".join(out)


def classify(out):
    if "Safety violation" in out:
        return "unreach-call"
    if "Non-atomic race" in out:
        return "atomic-race" if atomic_race(out) else "no-data-race"
    if re.search(r"access (freed|non-allocated) memory|Double-free|uninitialized memory"
                 r"|Attempt to access|Invalid free", out):
        return "valid-memsafety"
    if "Liveness violation" in out:
        return "liveness"
    if "unknown external function" in out:
        return "unsupported-call"
    if re.search(r"error:|Compilation failed|Errors detected", out):
        return "compile-error"
    if re.search(r"^Error", out, re.M):
        return "other-error"
    # e.g. "Warning: Unordered writes!" treated as an error: no verdict
    if "Verification unsuccesful" in out or "Estimation unsuccessful" in out:
        return "other-error"
    if "Number of complete executions explored" in out:
        return "ok"
    return "unknown"


def in_atomic_section(out, tid, idx):
    """Whether event (tid, idx) of the printed graph lies inside an atomic
    section, i.e. between taking and releasing __svc_atomic_lock."""
    inside = False
    for m in re.finditer(r"^\s*\((\d+), (\d+)\): (.*)$", out, re.M):
        if int(m.group(1)) != tid:
            continue
        if int(m.group(2)) == idx:
            return inside
        ev = m.group(3)
        if ev.startswith("CW") and re.search(r"\(__svc_atomic_lock[.\w]*, 1\)", ev):
            inside = True
        elif ev.startswith("W") and re.search(r"\(__svc_atomic_lock[.\w]*, 0\)", ev):
            inside = False
    return False


def atomic_race(out):
    """A race where one side is inside an atomic section. SV-COMP counts
    accesses in atomic sections as atomic, so this is not a race there; the
    shim's lock cannot express that."""
    m = re.search(r"Event \((\d+), (\d+)\) conflicts with event \((\d+), (\d+)\)", out)
    if not m:
        return False
    a, b, c, d = map(int, m.groups())
    return in_atomic_section(out, a, b) or in_atomic_section(out, c, d)


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
    src = os.path.join(d, name + ".c")
    if not os.path.exists(src):
        return (os.path.basename(d), name, expected, "no-c-source", 0.0, "NO-C-SOURCE", "")
    od = os.path.join(args.outdir, os.path.basename(d))
    os.makedirs(od, exist_ok=True)
    code = open(src, errors="replace").read()
    # SV-COMP runs functions named __VERIFIER_atomic_* atomically
    code = re.sub(r"(\b__VERIFIER_atomic_(?!begin\b|end\b)\w+\s*\([^;{)]*\)\s*\{)",
                  r"\1 SVC_ATOMIC_FUNCTION_GUARD;", code)
    # preprocessed tasks declare pthread_t themselves
    if not re.search(r"typedef\s+unsigned\s+long(\s+int)?\s+pthread_t\s*;", code):
        shim, mode = SHIM, "A"
        src = os.path.join(od, name + ".c")
        with open(src, "w") as f:
            f.write(code)
    else:
        shim, mode = SHIM_PP, "B"
        tail = [body for fn, body in TAIL.items()
                if re.search(r"\b" + fn + r"\s*\(", code)
                and not re.search(r"\b" + fn + r"\s*\([^;]*\)\s*\{", code)]
        src = os.path.join(od, name + ".pp.c")
        with open(src, "w") as f:
            f.write(code + "\n\n/* ---- svcomp-run tail ---- */\n" + "\n\n".join(tail) + "\n")
    t0 = time.time()
    try:
        main_argc = 1 if re.search(r"\bmain\s*\(\s*int\b", with_local_includes(code, d)) else 0
        recursive = 1 if "PTHREAD_MUTEX_RECURSIVE" in code else 0
        p = subprocess.run([args.genmc, f"-{args.model}", "-disable-estimation", "--",
                            "-I", os.path.abspath(d), f"-DSVC_MAIN_ARGC={main_argc}",
                            f"-DSVC_RECURSIVE_MUTEX={recursive}",
                            f"-DSVC_NONDET_MAX={args.nondet_max}",
                            f"-DSVC_MAX_CHOICES={args.max_choices}",
                            f"-DSVC_MAX_SPURIOUS={args.max_spurious}",
                            "-include", shim, src],
                           capture_output=True, text=True, timeout=args.timeout)
        out = p.stdout + p.stderr
        got = classify(out)
    except subprocess.TimeoutExpired:
        out, got = "", "timeout"
    dt = time.time() - t0
    with open(os.path.join(od, name + ".out"), "w") as f:
        f.write(out)
    first = next((l for l in out.splitlines()
                  if l.startswith(("Error", "ERROR")) or "error:" in l), "")
    return (os.path.basename(d), name, expected, got, dt, verdict(expected, got), first)


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
    ap.add_argument("genmc")
    ap.add_argument("dirs", nargs="+")
    args = ap.parse_args()

    jobs = [(d, y) for d in args.dirs for y in sorted(os.listdir(d)) if y.endswith(".yml")]
    rows = []
    with cf.ThreadPoolExecutor(args.j) as ex:
        for r in ex.map(lambda j: run_task(args, *j), jobs):
            if r:
                rows.append(r)

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
