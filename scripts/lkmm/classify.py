#!/usr/bin/env python3
"""Sort the disagreements of an lkmm-diff.py run (its --json output) by
likely cause, from the primitives a test uses.

The rules describe what a run under -imm with lkmm-c11.h is expected to
show (the C11 mapping is not LKMM); for a run of a real LKMM model every
disagreement is a bug until shown otherwise, and the rules are only hints.

Usage: classify.py <results.jsonl> <litmus-root> [--examples N]
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
    args = ap.parse_args()

    causes = collections.defaultdict(list)
    kinds = collections.Counter()
    for line in open(args.results):
        r = json.loads(line)
        kind = r["result"].split(":")[0]
        kinds[kind] += 1
        if kind in ("agree", "agree-racy", "skipped", "herd-timeout"):
            continue
        src = open(os.path.join(args.litmus_root, r["test"]), errors="replace").read()
        for ks, cause, pred in RULES:
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


if __name__ == "__main__":
    main()
