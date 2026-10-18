#!/usr/bin/env python3
"""Compare two lkmm-diff.py --json runs test by test: what changed, grouped
by (old result -> new result), with examples.

Usage: compare-runs.py <old.jsonl> <new.jsonl> [--examples N]

A test's result is its kind (agree, extra, ...); a timeout or an error
counts as a change only when the other run has a verdict. Exit status 1 if
a test got worse (agree/agree-racy -> anything else, or a new kind of
disagreement).
"""

import argparse
import collections
import json
import sys

GOOD = ("agree", "agree-racy")
UNDECIDED = ("genmc-timeout", "herd-timeout", "herd-error", "genmc-error")


def load(path):
    res = {}
    for line in open(path):
        r = json.loads(line)
        res[r["test"]] = r["result"].split(":")[0]
    return res


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("old")
    ap.add_argument("new")
    ap.add_argument("--examples", type=int, default=3)
    args = ap.parse_args()
    old, new = load(args.old), load(args.new)

    changes = collections.defaultdict(list)
    for t in sorted(set(old) | set(new)):
        a, b = old.get(t, "absent"), new.get(t, "absent")
        if a != b:
            changes[(a, b)].append(t)

    worse = 0
    print("| old | new | tests | examples |")
    print("|---|---|---|---|")
    for (a, b), tests in sorted(changes.items(), key=lambda kv: -len(kv[1])):
        bad = (a in GOOD and b not in GOOD) or (b not in GOOD and b not in UNDECIDED
                                                 and a in UNDECIDED)
        worse += len(tests) if bad else 0
        print("| %s | %s | %d%s | %s |" % (a, b, len(tests), " (worse)" if bad else "",
                                           ", ".join("`%s`" % t for t in tests[:args.examples])))
    same = len([t for t in old if new.get(t) == old[t]])
    print()
    print("same %d, changed %d, worse %d" % (same, sum(map(len, changes.values())), worse))
    sys.exit(1 if worse else 0)


if __name__ == "__main__":
    main()
