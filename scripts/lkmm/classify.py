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

RULES = [
    # (result kinds, cause, predicate on the test's source)
    (("count", "genmc-timeout"), "RCU emulation: synchronize_rcu() reads reader flags",
     lambda s: "synchronize_rcu" in s),
    (("missing",), "control dependency past the end of the if (IMM orders it, LKMM does not)",
     lambda s: re.search(r"\bif\s*\(", s) is not None),
    (("race",), "smp_rmb()/smp_wmb() as acquire/release fences order more than LKMM",
     lambda s: "smp_rmb" in s or "smp_wmb" in s),
    (("extra",), "spin_unlock(); spin_lock() (po-unlock-lock-po) is not ordered in C11",
     lambda s: "spin_lock" in s),
    (("extra",), "address dependency to a read (IMM's ppo ends in a write)",
     lambda s: re.search(r"(READ_ONCE|rcu_dereference)\s*\(\s*\*\s*\(", s) is not None
     or re.search(r"rcu_dereference", s) is not None),
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
            if kind in ks and pred(src):
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
