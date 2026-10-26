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
import re
import subprocess
import time

import yaml

import svcomp_witness

PROPS = {"unreach-call", "no-data-race", "valid-memsafety"}
# The specifications in SV-COMP's property files
SPECS = {
    "unreach-call": ["CHECK( init(main()), LTL(G ! call(reach_error())) )"],
    "no-data-race": ["CHECK( init(main()), LTL(G ! data-race) )"],
    "valid-memsafety": ["CHECK( init(main()), LTL(G valid-free) )",
                        "CHECK( init(main()), LTL(G valid-deref) )",
                        "CHECK( init(main()), LTL(G valid-memtrack) )"],
}
HERE = os.path.dirname(os.path.abspath(__file__))
SHIM = os.path.join(HERE, "svcomp-shim.h")
SHIM_PP = os.path.join(HERE, "svcomp-shim-pp.h")

# reach_error() of a task that defines it, in the header that GenMC reports
# at its caller's line
REACH_ERROR = """void reach_error(void)
{ __VERIFIER_assert_fail("reach_error()", __FILE__, __LINE__); }"""

# Definitions for preprocessed tasks, in terms of GenMC's primitives. Each is
# emitted only if the task mentions the function, so the types it uses exist.
TAIL = {
    # an atomic store, so that create_race() can tell it from the task's own
    "pthread_create": """int pthread_create(pthread_t *t, const pthread_attr_t *a,
		   void *(*f)(void *), void *arg)
{ __atomic_store_n(t, __VERIFIER_thread_create(0, f, arg), __ATOMIC_RELAXED); return 0; }""",
    "pthread_join": """int pthread_join(pthread_t t, void **r)
{ void *v = __VERIFIER_thread_join(t); if (r) *r = v; return 0; }""",
    "pthread_exit": "void pthread_exit(void *r) { __VERIFIER_thread_exit(r); }",
    "pthread_self": "pthread_t pthread_self(void) { return __VERIFIER_thread_self(); }",
    "pthread_mutex_init": """int pthread_mutex_init(pthread_mutex_t *m, const pthread_mutexattr_t *a)
{
	__svc_rec_register(m, a ? *(const int *) a : 0);
	return __VERIFIER_mutex_init((__VERIFIER_mutex_t *) m, 0);
}""",
    "pthread_mutex_destroy": """int pthread_mutex_destroy(pthread_mutex_t *m)
{ return __VERIFIER_mutex_destroy((__VERIFIER_mutex_t *) m); }""",
    "pthread_mutex_lock": """int pthread_mutex_lock(pthread_mutex_t *m)
{ return __svc_mutex_lock((__VERIFIER_mutex_t *) m); }""",
    "pthread_mutex_trylock": """int pthread_mutex_trylock(pthread_mutex_t *m)
{ return __svc_mutex_trylock((__VERIFIER_mutex_t *) m); }""",
    "pthread_mutex_unlock": """int pthread_mutex_unlock(pthread_mutex_t *m)
{ return __svc_mutex_unlock((__VERIFIER_mutex_t *) m); }""",
    "pthread_cond_init": """int pthread_cond_init(pthread_cond_t *c, const pthread_condattr_t *a)
{ __atomic_store_n((int *) c, 0, __ATOMIC_RELAXED); return 0; }""",
    "pthread_cond_destroy": "int pthread_cond_destroy(pthread_cond_t *c) { return 0; }",
    "pthread_cond_wait": """int pthread_cond_wait(pthread_cond_t *c, pthread_mutex_t *m)
{
	int seq = __atomic_load_n((int *) c, __ATOMIC_ACQUIRE);
	int spurious = __svc_spurious_wakeup();
	__svc_mutex_unlock((__VERIFIER_mutex_t *) m);
	if (!spurious)
		__VERIFIER_await(__atomic_load_n((int *) c, __ATOMIC_ACQUIRE) != seq);
	return __svc_mutex_lock((__VERIFIER_mutex_t *) m);
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
    # the attribute keeps the mutex type in its first int, which
    # pthread_mutex_init reads to tell recursive mutexes apart
    "pthread_mutexattr_init": "int pthread_mutexattr_init(pthread_mutexattr_t *a) { *(int *) a = 0; return 0; }",
    "pthread_mutexattr_destroy": "int pthread_mutexattr_destroy(pthread_mutexattr_t *a) { return 0; }",
    "pthread_mutexattr_settype": "int pthread_mutexattr_settype(pthread_mutexattr_t *a, int k) { *(int *) a = k; return 0; }",
    "pthread_mutexattr_gettype": """int pthread_mutexattr_gettype(const pthread_mutexattr_t *a, int *k)
{ *k = *(const int *) a; return 0; }""",
    # a rwlock's storage holds one GenMC mutex per thread (svcomp-shim-pp.h)
    "pthread_rwlock_init": """int pthread_rwlock_init(pthread_rwlock_t *l, const pthread_rwlockattr_t *a)
{
	for (unsigned long i = 0; i < SVC_RWLOCK_SLOTS; i++)
		__VERIFIER_mutex_init(&((__VERIFIER_mutex_t *) l)[i], 0);
	return 0;
}""",
    "pthread_rwlock_destroy": "int pthread_rwlock_destroy(pthread_rwlock_t *l) { return 0; }",
    "pthread_rwlock_rdlock": """int pthread_rwlock_rdlock(pthread_rwlock_t *l)
{ return __svc_rw_rdlock((__VERIFIER_mutex_t *) l, SVC_RWLOCK_SLOTS); }""",
    "pthread_rwlock_tryrdlock": """int pthread_rwlock_tryrdlock(pthread_rwlock_t *l)
{ return __svc_rw_tryrdlock((__VERIFIER_mutex_t *) l, SVC_RWLOCK_SLOTS); }""",
    "pthread_rwlock_wrlock": """int pthread_rwlock_wrlock(pthread_rwlock_t *l)
{ return __svc_rw_wrlock((__VERIFIER_mutex_t *) l, SVC_RWLOCK_SLOTS); }""",
    "pthread_rwlock_trywrlock": """int pthread_rwlock_trywrlock(pthread_rwlock_t *l)
{ return __svc_rw_trywrlock((__VERIFIER_mutex_t *) l, SVC_RWLOCK_SLOTS); }""",
    "pthread_rwlock_unlock": """int pthread_rwlock_unlock(pthread_rwlock_t *l)
{ return __svc_rw_unlock((__VERIFIER_mutex_t *) l, SVC_RWLOCK_SLOTS); }""",
    "pthread_key_create": """int pthread_key_create(pthread_key_t *k, void (*destructor)(void *))
{ return __svc_key_create((unsigned int *) k); }""",
    "pthread_key_delete": "int pthread_key_delete(pthread_key_t k) { return 0; }",
    "pthread_getspecific": "void *pthread_getspecific(pthread_key_t k) { return __svc_getspecific(k); }",
    "pthread_setspecific": """int pthread_setspecific(pthread_key_t k, const void *p)
{ return __svc_setspecific(k, p); }""",
    # GenMC's allocator rejects size 0 (its stdlib.h asserts); malloc(0) is
    # legal C, so drop those executions instead of reporting an error
    # and it may fail (svcomp-shim-core.h)
    "malloc": """void *malloc(size_t n)
{ __VERIFIER_assume(n > 0); return __svc_malloc_fails() ? 0 : __VERIFIER_malloc(n); }""",
    "calloc": """void *calloc(size_t n, size_t sz)
{
	__VERIFIER_assume(n > 0 && sz > 0);
	return __svc_malloc_fails() ? 0 : __VERIFIER_calloc(n, sz);
}""",
    "free": "void free(void *p) { __VERIFIER_free(p); }",
    # SV-COMP's abort() cuts off a path; GenMC's real abort() would kill GenMC
    "abort": "void abort(void) { __svc_abort_path(); }",
    "pthread_detach": "int pthread_detach(pthread_t t) { return 0; }",
    # sleeping only delays a thread, and GenMC explores every interleaving
    "sleep": "unsigned int sleep(unsigned int s) { return 0; }",
    "usleep": "int usleep(unsigned int us) { return 0; }",
    "strcpy": """char *strcpy(char *d, const char *s)
{ char *r = d; while ((*d++ = *s++) != 0) ; return r; }""",
    "ffs": """int ffs(int i)
{
	unsigned int u = i;
	int n = 1;

	if (!u)
		return 0;
	while (!(u & 1)) {
		u >>= 1;
		n++;
	}
	return n;
}""",
    "sem_init": """int sem_init(sem_t *s, int pshared, unsigned int v)
{ __atomic_store_n((int *) s, (int) v, __ATOMIC_RELAXED); return 0; }""",
    "sem_destroy": "int sem_destroy(sem_t *s) { return 0; }",
    "sem_wait": "int sem_wait(sem_t *s) { return __svc_sem_wait((int *) s); }",
    "sem_trywait": "int sem_trywait(sem_t *s) { return __svc_sem_trywait((int *) s); }",
    "sem_post": """int sem_post(sem_t *s)
{ __atomic_fetch_add((int *) s, 1, __ATOMIC_RELEASE); return 0; }""",
    "sem_getvalue": """int sem_getvalue(sem_t *s, int *v)
{ *v = __atomic_load_n((int *) s, __ATOMIC_RELAXED); return 0; }""",
    "__assert_fail": """void __assert_fail(const char *e, const char *file, unsigned int line,
		   const char *fn)
{ __VERIFIER_assert_fail(e, file, line); }""",
}


# The parameter list of a function definition: no ';' or braces, and
# parentheses nested one level deep, as in void *(*f)(void *)
PARAMS = r"\((?:[^;{}()]|\([^;{}()]*\))*\)"


def defines(code, fn):
    """Whether CODE defines function FN: a type word before the name, and a
    body after the parameters. A call such as `if (pthread_create(...)) {`
    has a '(' before the name, so it does not count."""
    return re.search(r"\b[A-Za-z_]\w*[\s*]+" + fn + r"\s*" + PARAMS + r"\s*\{", code)


def mentions_recursive(code):
    """Whether CODE uses recursive mutexes. A preprocessed task carries the
    PTHREAD_MUTEX_RECURSIVE enumerators of <pthread.h> whether it uses
    them or not, so those declarations do not count."""
    code = re.sub(r"\bPTHREAD_MUTEX_RECURSIVE(_NP)?\s*(=\s*\w+\s*)?,", "", code)
    return re.search(r"\bPTHREAD_MUTEX_RECURSIVE(_NP)?\b", code) is not None


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
        if atomic_race(out):
            return "atomic-race"
        return "create-race" if create_race(out) else "no-data-race"
    if re.search(r"access (freed|non-allocated) memory|Double-free"
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


def memsafety_subproperty(out):
    """The subproperty of valid-memsafety that GenMC's memory error violates"""
    return "valid-free" if re.search(r"Double-free|Invalid free", out) else "valid-deref"


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


def create_race(out):
    """A race on the store of a new thread's ID: pthread_create's store right
    after the THREAD_CREATE of that thread. The shim and the generated tail
    store the ID atomically, at a line of svcomp-shim.h or at the line of
    the THREAD_CREATE (the tail's pthread_create), so that the store is
    told from a store of the task's own, which may store the same value
    right after pthread_create and is printed without a location when it
    is the racing event. glibc stores the ID before the thread runs, so a
    read ordered after the thread's start does not race with it there;
    GenMC creates the thread first."""
    m = re.search(r"Event \((\d+), (\d+)\) conflicts with event \((\d+), (\d+)\)", out)
    if not m:
        return False
    events = {(int(e[0]), int(e[1])): e[2]
              for e in re.findall(r"^\s*\((\d+), (\d+)\): (.*)$", out, re.M)}

    def is_id_store(tid, idx):
        create = re.fullmatch(r"THREAD_CREATE \[thread (\d+)\]\s*(\S*)",
                              events.get((tid, idx - 1), ""))
        store = re.fullmatch(r"W(?!na\b)\w* \(.*, (\d+)\)\s*(\S+)", events.get((tid, idx), ""))
        return bool(create and store and create.group(1) == store.group(1)
                    and (store.group(2) == create.group(2)
                         or store.group(2).startswith("svcomp-shim.h:")))

    a, b, c, d = map(int, m.groups())
    return is_id_store(a, b) or is_id_store(c, d)


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
    task_src = src
    od = os.path.join(args.outdir, os.path.basename(d))
    os.makedirs(od, exist_ok=True)
    code = open(src, errors="replace").read()
    # SV-COMP runs functions named __VERIFIER_atomic_* atomically
    code = re.sub(r"(\b__VERIFIER_atomic_(?!begin\b|end\b)\w+\s*\([^;{)]*\)\s*\{)",
                  r"\1 SVC_ATOMIC_FUNCTION_GUARD;", code)
    # preprocessed tasks declare pthread_t themselves (unsigned long, or int
    # in pthread-driver-races)
    # The code the runner adds goes into a header of its own after the task,
    # which GenMC reports at the line of the task that called it
    # (-library-dir): the lines GenMC reports are the task's, as a witness
    # needs. reach_error() moves there too, so that an error is reported
    # at the call that reached it, not in reach_error()'s body
    lib = []
    # (keeping the definition's line breaks, so that the lines stay the task's)
    code, renamed = re.subn(r"\bvoid\s+reach_error\s*\(\s*(void)?\s*\)\s*\{",
                            lambda m: "void reach_error(void); void __svc_task_reach_error(void) {"
                            + "\n" * m.group(0).count("\n"), code, count=1)
    if renamed:
        lib.append(REACH_ERROR)
    if not re.search(r"\btypedef\s+[^;{}]+\bpthread_t\s*;", code):
        shim, mode = SHIM, "A"
        src = os.path.join(od, name + ".c")
    else:
        shim, mode = SHIM_PP, "B"
        lib += [body for fn, body in TAIL.items()
                if re.search(r"\b" + fn + r"\s*\(", code) and not defines(code, fn)]
        src = os.path.join(od, name + ".pp.c")
    libdir = os.path.join(od, name + ".lib")
    os.makedirs(libdir, exist_ok=True)
    with open(os.path.join(libdir, "svcomp-lib.h"), "w") as f:
        f.write("/* added by svcomp-run.py to " + os.path.basename(task_src) + " */\n"
                + "\n\n".join(lib) + "\n")
    with open(src, "w") as f:
        f.write(code + '\n#include "' + os.path.abspath(libdir) + '/svcomp-lib.h"\n')
    main_argc = 1 if re.search(r"\bmain\s*\(\s*int\b", with_local_includes(code, d)) else 0
    recursive = 1 if mentions_recursive(code) else 0
    # SV-COMP's malloc may fail; only memory safety is about that
    malloc_fail = 1 if "valid-memsafety" in expected else 0

    def genmc(check_bounds, timeout):
        p = subprocess.run([args.genmc, f"-{args.model}", "-disable-estimation",
                            *args.genmc_flags,
                            *(f"-library-dir={x}" for x in (HERE, os.path.abspath(libdir))
                              if args.library_dirs), "--",
                            "-I", os.path.abspath(d), f"-DSVC_MAIN_ARGC={main_argc}",
                            f"-DSVC_RECURSIVE_MUTEX={recursive}",
                            f"-DSVC_NONDET_MAX={args.nondet_max}",
                            f"-DSVC_MAX_CHOICES={args.max_choices}",
                            f"-DSVC_MAX_SPURIOUS={args.max_spurious}",
                            f"-DSVC_CHECK_BOUNDS={check_bounds}",
                            f"-DSVC_MALLOC_FAIL={malloc_fail}",
                            # gcc, which SV-COMP's tasks are written for,
                            # only warns about these
                            "-Wno-error=int-conversion",
                            "-Wno-error=incompatible-pointer-types",
                            "-include", shim, src],
                           capture_output=True, text=True, timeout=timeout)
        return p.stdout + p.stderr

    t0 = time.time()
    try:
        out = genmc(0, args.timeout)
        got = classify(out)
        # a clean result holds only if no execution was cut by the shim's
        # bounds: rerun to see whether one was (svcomp-shim-core.h)
        if got == "ok" and "Number of blocked executions seen" in out:
            check = genmc(1, max(1, args.timeout - (time.time() - t0)))
            out += "\n---- bounds check ----\n" + check
            if "__svc_bound_hit" in check:
                got = "bounded"
            elif classify(check) != "ok":
                got = "unknown"
    except subprocess.TimeoutExpired:
        out, got = "", "timeout"
    if args.witness and got in PROPS:
        prp = next(p["property_file"] for p in task["properties"]
                   if p["property_file"].endswith("/" + got + ".prp"))
        # (SV-COMP's text, if the task's property file is not at hand)
        path = os.path.join(d, prp)
        checks = ([l.strip() for l in open(path) if l.startswith("CHECK")]
                  if os.path.exists(path) else SPECS[got])
        # valid-memsafety lists its subproperties: the witness names the violated one
        sub = memsafety_subproperty(out) if got == "valid-memsafety" else None
        spec = next((c for c in checks if sub and "G " + sub + ")" in c), checks[0])
        svcomp_witness.write_witnesses(out, os.path.basename(src), task_src, spec,
                                       task.get("options", {}).get("data_model", "ILP32"),
                                       os.path.join(od, name + ".witness"))
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
    ap.add_argument("--witness", action="store_true",
                    help="write OUTDIR/<dir>/<task>.witness.{graphml,yml} for each error")
    ap.add_argument("genmc")
    ap.add_argument("dirs", nargs="+")
    args = ap.parse_args()
    help_text = subprocess.run([args.genmc, "--help"], capture_output=True, text=True).stdout
    args.library_dirs = re.search(r"^\s*--?library-dir\b", help_text, re.M) is not None
    args.genmc_flags = ["-svcomp"] if re.search(r"^\s*--?svcomp\b", help_text, re.M) else []

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
