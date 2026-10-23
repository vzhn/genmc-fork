#!/usr/bin/env python3
#
# Reads `llvm-cov export -format=text -summary-only` on stdin and prints line,
# region and branch coverage per source directory (relative to ROOT, the
# first argument): one row per layer and group of genmc/genmc, lli and passes.
#
# This program is dual-licensed under the Apache License 2.0 and the MIT License.
# You may choose to use, distribute, or modify this software under either license.
import json
import os
import sys
from collections import defaultdict

root = os.path.realpath(sys.argv[1])
data = json.load(sys.stdin)["data"][0]
total = defaultdict(lambda: [0, 0])
dirs = defaultdict(lambda: defaultdict(lambda: [0, 0]))
for f in data["files"]:
    path = os.path.relpath(os.path.realpath(f["filename"]), root)
    d = os.path.dirname(path)
    for kind in ("lines", "regions", "branches"):
        s = f["summary"][kind]
        dirs[d][kind][0] += s["covered"]
        dirs[d][kind][1] += s["count"]
        total[kind][0] += s["covered"]
        total[kind][1] += s["count"]


def pct(c):
    return f"{100.0 * c[0] / c[1]:5.1f}%" if c[1] else "    -"


print(f"{'directory':44} {'lines':>16} {'regions':>16} {'branches':>16}")
for d in sorted(dirs):
    k = dirs[d]
    print(f"{d:44} {pct(k['lines']):>7} {k['lines'][1]:>8} {pct(k['regions']):>7} {k['regions'][1]:>8} "
          f"{pct(k['branches']):>7} {k['branches'][1]:>8}")
print(f"{'total':44} {pct(total['lines']):>7} {total['lines'][1]:>8} {pct(total['regions']):>7} "
      f"{total['regions'][1]:>8} {pct(total['branches']):>7} {total['branches'][1]:>8}")
