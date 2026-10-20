#!/usr/bin/env python3
"""Random LKMM litmus tests with branches of different lengths (for
GenMC's scoped control dependencies against herd7).

    scripts/lkmm/fuzz-ctrl.py <dir> <first-seed> <count>
    scripts/lkmm/run-herd.sh <model-dir> <herd-out> <dir> <dir>/*.litmus
    scripts/lkmm/lkmm-diff.py --strict-counts --herd-out <herd-out> --litmus-root <dir> <dir>

Seeds are stable: fuzz-<seed>.litmus is the same test on every run."""
import random, sys, os

LOCS = ["x", "y", "z", "w"]

def gen_test(seed):
    rnd = random.Random(seed)
    nthreads = rnd.choice([2, 2, 3])
    threads = []
    regs_all = []
    for t in range(nthreads):
        regs = []
        lines = []
        def newreg():
            r = "r%d" % len(regs)
            regs.append(r)
            return r
        def stmt(depth, budget):
            k = rnd.random()
            loc = rnd.choice(LOCS)
            if k < 0.30 or budget <= 0:
                v = rnd.choice([1, 2])
                if rnd.random() < 0.15:
                    return ["smp_store_release(%s, %d);" % (loc, v)]
                return ["WRITE_ONCE(*%s, %d);" % (loc, v)]
            if k < 0.50:
                r = newreg()
                if rnd.random() < 0.15:
                    return ["%s = smp_load_acquire(%s);" % (r, loc)]
                return ["%s = READ_ONCE(*%s);" % (r, loc)]
            if k < 0.60 and depth < 2:
                f = rnd.choice(["smp_mb();", "smp_wmb();", "smp_rmb();"])
                return [f]
            if k < 0.65:
                r = newreg()
                return ["%s = cmpxchg_relaxed(%s, 0, %d);" % (r, loc, rnd.choice([1, 2]))]
            if depth < 2 and regs:
                r = rnd.choice(regs)
                body = []
                for _ in range(rnd.randint(1, 2)):
                    body += stmt(depth + 1, budget - 1)
                els = []
                if rnd.random() < 0.4:
                    for _ in range(rnd.randint(1, 3)):
                        els += stmt(depth + 1, budget - 1)
                out = ["if (%s) {" % r] + ["\t" + l for l in body] + ["}"]
                if els:
                    out = out[:-1] + ["} else {"] + ["\t" + l for l in els] + ["}"]
                return out
            return ["WRITE_ONCE(*%s, %d);" % (loc, rnd.choice([1, 2]))]
        # start with a read, so there is something to branch on
        r = newreg()
        lines.append("%s = READ_ONCE(*%s);" % (r, rnd.choice(LOCS)))
        for _ in range(rnd.randint(2, 4)):
            lines += stmt(0, 3)
        threads.append((regs, lines))
        regs_all.append(regs)
    used = sorted({l for _, ls in threads for line in ls for l in LOCS
                   if ("*%s" % l) in line or ("(%s," % l) in line or ("(%s)" % l) in line})
    out = ["C fuzz-%d" % seed, "", "{}", ""]
    for t, (regs, lines) in enumerate(threads):
        out.append("P%d(%s)" % (t, ", ".join("int *%s" % l for l in LOCS)))
        out.append("{")
        for r in regs:
            out.append("\tint %s;" % r)
        out.append("")
        for l in lines:
            out.append("\t" + l)
        out.append("}")
        out.append("")
    conds = ["%d:%s=1" % (t, regs[0]) for t, regs in enumerate(regs_all)]
    out.append("exists (%s)" % " /\\ ".join(conds))
    return "\n".join(out) + "\n"

if __name__ == "__main__":
    d, start, n = sys.argv[1], int(sys.argv[2]), int(sys.argv[3])
    os.makedirs(d, exist_ok=True)
    for s in range(start, start + n):
        open(os.path.join(d, "fuzz-%d.litmus" % s), "w").write(gen_test(s))
