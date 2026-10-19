#!/usr/bin/env python3
"""Sort the disagreements of an lkmm-diff.py run (its --json output) by
likely cause, from the primitives a test uses.

The default rules describe what a run under -imm with lkmm-c11.h is
expected to show (the C11 mapping is not LKMM). --model lkmm selects the
rules for a run of the -lkmm model: its one permanent gap (control
dependencies past the if), herd7's symbolic thin-air values, and what
herd7 itself rejects; every other disagreement is a bug.
--allowlist writes the classified disagreements in lkmm-diff.py's
--allowlist format, so that a rerun fails only on new ones.

Usage: classify.py <results.jsonl> <litmus-root> [--model lkmm] [--examples N]
                   [--allowlist FILE]
"""

import argparse
import collections
import json
import os
import re

def same_location_writes(src):
    """Whether a thread writes the same location twice"""
    for body in re.findall(r"P\d+\s*\([^)]*\)\s*\{(.*?)\n\}", src, re.S):
        locs = re.findall(r"WRITE_ONCE\s*\(\s*\*\s*(\w+)", body)
        if len(locs) != len(set(locs)):
            return True
    return False


def symbolic(r):
    """Whether a missing state has one of herd7's symbolic values (S<n>)"""
    return any(re.fullmatch(r"S\d+", v) for st in r.get("missing", []) for v in st.values())


LKMM_RULES = [
    (("missing",), "herd7's symbolic value (S<n>) of a thin-air cycle through plain "
     "accesses: no execution of the C program has it",
     lambda s, r: symbolic(r)),
    (("missing",), "control dependency past the end of the if: GenMC keeps every later "
     "event dependent on the branch (its revisit prefixes need it; plan 3.6), LKMM does not",
     lambda s, r: re.search(r"\bif\s*\(", s) is not None),
    (("race",), "control dependency past the end of the if hides the racy execution "
     "(GenMC does not explore it)",
     lambda s, r: not r.get("genmc_race") and re.search(r"\bif\s*\(", s) is not None),
    (("count",), "locks: GenMC's mutex events, not herd7's lock events (same states)",
     lambda s, r: "spin_lock" in s),
    (("count",), "same states, other execution count (failed cmpxchg reads); not a "
     "disagreement without --strict-counts",
     lambda s, r: True),
    (("herd-error",), "herd7 rejects the test (unknown macro or annotation)",
     lambda s, r: True),
]

RULES = [
    # (result kinds, cause, predicate on (source, result))
    (("count", "genmc-timeout"), "RCU emulation: synchronize_rcu() reads reader flags",
     lambda s, r: "synchronize_rcu" in s),
    (("count",), "locks: pthread mutex events, not herd7's lock events",
     lambda s, r: "spin_lock" in s),
    (("missing",), "RCU emulation: rcu_read_lock()/unlock() fence, LKMM's do not order",
     lambda s, r: "rcu_read_lock" in s),
    (("missing",), "control dependency past the end of the if (IMM orders it, LKMM does not)",
     lambda s, r: re.search(r"\bif\s*\(", s) is not None),
    (("race", "missing"), "smp_rmb()/smp_wmb() as acquire/release fences order more than LKMM",
     lambda s, r: not r.get("genmc_race") and ("smp_rmb" in s or "smp_wmb" in s)),
    (("race",), "control dependency past the end of the if hides the racy execution",
     lambda s, r: not r.get("genmc_race") and re.search(r"\bif\s*\(", s) is not None),
    (("race",), "LKMM orders plain accesses by dependencies and fences C11 ignores",
     lambda s, r: r.get("genmc_race")),
    (("extra",), "spin_unlock(); spin_lock() (po-unlock-lock-po) is not ordered in C11",
     lambda s, r: "spin_lock" in s),
    (("extra",), "address dependency to a read (IMM's ppo ends in a write)",
     lambda s, r: re.search(r"rcu_dereference|READ_ONCE\s*\(\s*\*\s*(\(|r\w*\s*\))", s)
     is not None),
    (("extra",), "same-thread overwrite after a dependency (LKMM's ppo has overwrite & int)",
     lambda s, r: same_location_writes(s)),
    (("extra",), "strong-fence cumulativity (smp_mb(), synchronize_rcu()): LKMM propagates "
     "writes, C11 needs the reader to synchronise",
     lambda s, r: "smp_mb" in s or "synchronize_rcu" in s),
    (("extra",), "release chains are A-cumulative in LKMM (prop), also through relaxed reads",
     lambda s, r: "smp_store_release" in s),
    (("count",), "same states, different model (the C11 mapping) explores other executions",
     lambda s, r: True),
    (("herd-error",), "herd7 rejects the test (unknown macro or annotation)",
     lambda s, r: True),
]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("results")
    ap.add_argument("litmus_root")
    ap.add_argument("--examples", type=int, default=3)
    ap.add_argument("--model", default="imm", choices=("imm", "lkmm"))
    ap.add_argument("--allowlist", help="write the classified disagreements here, in "
                    "lkmm-diff.py's --allowlist format")
    args = ap.parse_args()
    rules = LKMM_RULES if args.model == "lkmm" else RULES

    causes = collections.defaultdict(list)
    kinds = collections.Counter()
    for line in open(args.results):
        r = json.loads(line)
        kind = r["result"].split(":")[0]
        kinds[kind] += 1
        if kind in ("agree", "agree-racy", "skipped", "herd-timeout"):
            continue
        src = open(os.path.join(args.litmus_root, r["test"]), errors="replace").read()
        for ks, cause, pred in rules:
            if kind in ks and pred(src, r):
                causes[(kind, cause)].append(r["test"])
                break
        else:
            causes[(kind, "unclassified")].append(r["test"])

    print("results: " + ", ".join("%s %d" % kv for kv in kinds.most_common()))
    print()
    print("| result | cause | tests | examples |")
    print("|---|---|---|---|")
    for (kind, cause), tests in sorted(causes.items(), key=lambda kv: (kv[0][0], -len(kv[1]))):
        print("| %s | %s | %d | %s |" % (kind, cause, len(tests),
                                         ", ".join("`%s`" % t for t in tests[:args.examples])))
    if args.allowlist:
        with open(args.allowlist, "w") as f:
            f.write("# Classified disagreements of lkmm-diff.py --model %s (classify.py --allowlist)\n"
                    "# <test> <result> <cause>\n" % args.model)
            for (kind, cause), tests in sorted(causes.items()):
                if cause == "unclassified":
                    continue
                for t in sorted(tests):
                    f.write("%s %s %s\n" % (t, kind, cause))


if __name__ == "__main__":
    main()
