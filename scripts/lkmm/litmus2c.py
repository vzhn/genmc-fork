#!/usr/bin/env python3
"""Translate a C litmus test (herd7's C dialect, as used by the Linux-kernel
memory model) into a C program that GenMC can run.

Shared locations become globals named __lk_<name>; each thread P<i> becomes a
pthread whose pointer parameters point at those globals; registers become
per-thread globals __P<i>_<name>, so that main can read them after joining the
threads (with --local-regs, locals that a thread copies from and back to those
globals: a register a branch sets is then a value, not memory, see issue #78).
main then marks the end of the execution with __VERIFIER_output() and
writes the value of every observable (the locations and registers of the
final condition and of `locations [...]`) to one volatile variable, in a fixed
order. With -print-exec-graphs, those writes give the final state of every
execution GenMC explores (see lkmm-diff.py). A `filter` becomes an assumption.

Addresses are written as sentinels (ADDR_BASE - k for the k-th location), so
that a state can name the location a pointer points to, as herd7 does.

Usage: litmus2c.py [--header H] <test.litmus> [-o out.c] [--meta out.json]

Exits with 3 and prints "unsupported: <reason>" for constructs it does not
handle; it never produces a translation it knows to be wrong.
"""

import argparse
import json
import re
import sys

ADDR_BASE = -1000000
OUTPUT_MARK = "lkmm-state"

UNSUPPORTED_CALLS = ["smp_cond_load"]
# lkmm.h provides SRCU under -lkmm only
SRCU_CALLS = [
    "srcu_read_lock", "srcu_read_unlock", "srcu_down_read", "srcu_up_read",
    "synchronize_srcu", "smp_mb__after_srcu_read_unlock",
]

SCALAR_TYPES = r"(?:intptr_t|uintptr_t|int|long|short|char|unsigned|signed|bool|u8|u16|u32|u64|s32|s64)"
TYPE_RE = r"(?:(?:volatile|const|unsigned|signed)\s+)*" + SCALAR_TYPES + r"(?:\s+(?:int|long))*"


class Unsupported(Exception):
    pass


def strip_comments(s):
    """Removes C comments everywhere and herd's (* ... *) comments outside
    threads (inside a thread, "(*" is C, as in READ_ONCE(*x)); the first
    top-level brace block is the init section, where (* ... *) is a comment"""
    out, i, depth, blocks = [], 0, 0, 0
    while i < len(s):
        if s.startswith("//", i):
            j = s.find("\n", i)
            i = len(s) if j < 0 else j
        elif s.startswith("/*", i):
            j = s.find("*/", i + 2)
            i = len(s) if j < 0 else j + 2
            out.append(" ")
        elif (depth == 0 or blocks == 1) and s.startswith("(*", i):
            j = s.find("*)", i + 2)
            i = len(s) if j < 0 else j + 2
            out.append(" ")
        else:
            ch = s[i]
            if ch == "{" and depth == 0:
                blocks += 1
            depth += {"{": 1, "}": -1}.get(ch, 0)
            out.append(ch)
            i += 1
    return "".join(out)


def match_brace(s, i):
    """Index just past the brace block that starts at s[i] == '{'"""
    assert s[i] == "{"
    depth = 0
    for j in range(i, len(s)):
        if s[j] == "{":
            depth += 1
        elif s[j] == "}":
            depth -= 1
            if depth == 0:
                return j + 1
    raise Unsupported("unbalanced braces")


def split_top(s, sep):
    """Split s on sep at parenthesis depth 0"""
    parts, depth, cur = [], 0, ""
    for ch in s:
        if ch in "([{":
            depth += 1
        elif ch in ")]}":
            depth -= 1
        if ch == sep and depth == 0:
            parts.append(cur)
            cur = ""
        else:
            cur += ch
    parts.append(cur)
    return parts


def widen_int(s):
    """herd7's int is a machine word, which can hold an address: make every
    int an intptr_t, so a location written with a pointer has one size"""
    s = re.sub(r"(?<!unsigned )(?<!long )\bint\b(?!\s+(?:long|short))", "intptr_t", s)
    # an srcu_struct is a word too (lkmm.h's SRCU accesses its first int)
    return re.sub(r"\bstruct\s+srcu_struct\b", "intptr_t", s)


class Litmus:
    def __init__(self, text):
        self.raw = text
        lines = text.lstrip().split("\n", 1)
        head = lines[0].split()
        if not head or head[0] != "C":
            raise Unsupported("not a C litmus test (%s)" % (head[0] if head else "empty"))
        self.name = head[1] if len(head) > 1 else "test"
        m = re.search(r"Result:\s*(\w+)", text)
        self.expected = m.group(1) if m else None
        body = strip_comments(lines[1] if len(lines) > 1 else "")

        i = body.find("{")
        if i < 0:
            raise Unsupported("no init section")
        j = match_brace(body, i)
        self.init_text = widen_int(body[i + 1:j - 1])
        rest = body[j:]

        self.threads = []  # (index, params text, body text)
        pos = 0
        for m in re.finditer(r"\bP(\d+)\s*\(([^)]*)\)\s*\{", rest):
            if m.start() < pos:
                continue
            k = m.end() - 1
            end = match_brace(rest, k)
            self.threads.append((int(m.group(1)), widen_int(m.group(2)),
                                 widen_int(rest[k + 1:end - 1])))
            pos = end
        if not self.threads:
            raise Unsupported("no threads")
        idx = [t[0] for t in self.threads]
        if idx != list(range(len(idx))):
            raise Unsupported("threads are not P0..Pn in order")
        self.final_text = rest[pos:]


def parse_value(v):
    v = v.strip()
    if re.fullmatch(r"-?\d+", v):
        return ("int", int(v))
    m = re.fullmatch(r"ATOMIC_INIT\s*\(\s*(-?\d+)\s*\)", v)
    if m:
        return ("int", int(m.group(1)))
    m = re.fullmatch(r"&?\s*([A-Za-z_]\w*)", v)
    if m:
        return ("addr", m.group(1))
    raise Unsupported("init value %r" % v)


class Translator:
    def __init__(self, lt, header, srcu=False, local_regs=False):
        self.local_regs = local_regs
        self.lt = lt
        self.header = header
        self.unsupported = UNSUPPORTED_CALLS + ([] if srcu else SRCU_CALLS)
        self.locs = {}        # name -> declared type of the location
        self.loc_init = {}    # name -> ("int", n) | ("addr", name)
        self.regs = {}        # (tid, name) -> type
        self.reg_init = {}    # (tid, name) -> value
        self.thread_params = {}

    # -- types --------------------------------------------------------------

    def note_loc(self, name, typ=None):
        if typ is not None and name not in self.locs:
            self.locs[name] = typ
        else:
            self.locs.setdefault(name, None)

    def parse_params(self):
        for tid, params, _ in self.lt.threads:
            plist = []
            for p in split_top(params, ","):
                p = p.strip()
                if not p:
                    continue
                m = re.fullmatch(r"(.*?)\s*\*\s*([A-Za-z_]\w*)", p, flags=re.S)
                if not m:
                    raise Unsupported("parameter %r" % p)
                base, name = m.group(1).strip(), m.group(2)
                base = re.sub(r"\s+", " ", base)
                plist.append((name, base))
                self.note_loc(name, base)
            self.thread_params[tid] = plist

    def parse_init(self):
        for item in re.split(r"[;\n]", self.lt.init_text):
            item = item.strip()
            if not item:
                continue
            # [type] T:reg [= value]
            m = re.fullmatch(r"(?:([A-Za-z_][\w ]*?\**)\s*)?(\d+):([A-Za-z_]\w*)\s*(?:=\s*(.+))?",
                             item)
            if m:
                tid, reg = int(m.group(2)), m.group(3)
                typ = (m.group(1) or "intptr_t").strip()
                self.regs.setdefault((tid, reg), typ)
                if m.group(4) is not None:
                    self.reg_init[(tid, reg)] = parse_value(m.group(4))
                continue
            # [type] name [= value]
            m = re.fullmatch(r"(?:([A-Za-z_][\w ]*?)\s*(\**)\s+|([A-Za-z_][\w ]*?)\s*(\*+)\s*)?"
                             r"([A-Za-z_]\w*)\s*(?:=\s*(.+))?", item)
            if not m:
                raise Unsupported("init item %r" % item)
            typ = None
            if m.group(1):
                typ = m.group(1) + (" " + m.group(2) if m.group(2) else "")
            elif m.group(3):
                typ = m.group(3) + " " + m.group(4)
            name = m.group(5)
            if typ is not None:
                typ = re.sub(r"\s+", " ", typ).strip()
                self.locs[name] = typ
            else:
                self.note_loc(name)
            if m.group(6) is not None:
                val = parse_value(m.group(6))
                self.loc_init[name] = val
                if val[0] == "addr":
                    self.note_loc(val[1])

    # -- threads ------------------------------------------------------------

    def rewrite_body(self, tid, body):
        for call in self.unsupported:
            if re.search(r"\b" + call + r"\w*\s*\(", body):
                raise Unsupported("uses " + call)
        if re.search(r"\b(while|for|goto|do)\b", body):
            raise Unsupported("loops")
        if re.search(r"\b(struct|union)\b", body):
            raise Unsupported("struct in thread body")

        decl = re.compile(r"((?:(?<=[;{}])|^)\s*)(" + TYPE_RE + r")\b(?!\s*\))([^;()]*(?:\([^;]*\))?[^;]*);")

        def repl(m):
            lead, typ, decls = m.group(1), re.sub(r"\s+", " ", m.group(2)), m.group(3)
            out = []
            for d in split_top(decls, ","):
                dm = re.fullmatch(r"\s*(\**)\s*([A-Za-z_]\w*)\s*(?:=\s*(.*\S))?\s*", d, flags=re.S)
                if not dm:
                    raise Unsupported("declaration %r" % d.strip())
                stars, name, init = dm.groups()
                full = typ + (" " + stars if stars else "")
                prev = self.regs.get((tid, name))
                if prev is not None and prev.replace(" ", "") != full.replace(" ", ""):
                    raise Unsupported("register %d:%s declared twice" % (tid, name))
                self.regs[(tid, name)] = full
                if init is not None:
                    out.append("%s = %s;" % (name, init))
            return lead + " ".join(out)

        body = decl.sub(repl, body)

        # herd7 lets a register be assigned without a declaration
        params = {name for name, _ in self.thread_params[tid]}
        for m in re.finditer(r"(?:^|[;{}])\s*([A-Za-z_]\w*)\s*=(?!=)", body):
            name = m.group(1)
            if name not in params and (tid, name) not in self.regs:
                self.regs[(tid, name)] = "intptr_t"

        # herd7's C is loosely typed: a scalar register can be dereferenced
        for (t, r), typ in self.regs.items():
            if t != tid or "*" in typ:
                continue
            body = re.sub(r"(?<![\w)\]\s])(\s*)\*(\s*)%s\b" % r,
                          r"\1*\2(intptr_t *) %s" % r, body)
        return body

    # -- final section --------------------------------------------------------

    def parse_final(self):
        text = self.lt.final_text
        self.observables = []
        self.filter = None
        m = re.search(r"\blocations\s*\[([^\]]*)\]", text)
        if m:
            for o in m.group(1).split(";"):
                o = o.strip()
                if o:
                    self.add_observable(o)
            text = text[:m.start()] + text[m.end():]
        m = re.search(r"\bfilter\b", text)
        if m:
            k = text.index("(", m.end())
            depth, j = 0, k
            while True:
                depth += {"(": 1, ")": -1}.get(text[j], 0)
                if depth == 0:
                    break
                j += 1
            self.filter = text[k:j + 1]
            text = text[:m.start()] + text[j + 1:]
        m = re.search(r"(~\s*exists|exists|forall)\b(.*)", text, flags=re.S)
        if not m:
            raise Unsupported("no final condition")
        self.quantifier = re.sub(r"\s+", "", m.group(1))
        self.condition = m.group(2).strip()
        for o in self.atoms(self.condition):
            self.add_observable(o)
        if self.filter:
            for o in self.atoms(self.filter):
                pass  # filter atoms are evaluated in C, not observed

    def atoms(self, cond):
        if re.search(r"\[|!=|<|>|\+", cond):
            raise Unsupported("condition syntax %r" % cond.strip())
        atoms = []
        for lhs, rhs in re.findall(r"(\w+(?::\w+)?)\s*=\s*(-?\w+(?::\w+)?)", cond):
            atoms.append(lhs)
            if re.fullmatch(r"\d+:\w+", rhs):
                atoms.append(rhs)  # a register compared with a register
        return atoms

    def add_observable(self, o):
        if o in self.observables:
            return
        m = re.fullmatch(r"(\d+):([A-Za-z_]\w*)", o)
        if m:
            key = (int(m.group(1)), m.group(2))
            if key[0] >= len(self.lt.threads):
                raise Unsupported("condition names thread %d" % key[0])
            self.regs.setdefault(key, "intptr_t")
        elif re.fullmatch(r"[A-Za-z_]\w*", o):
            self.note_loc(o)
        else:
            raise Unsupported("observable %r" % o)
        self.observables.append(o)

    def c_cond(self, cond):
        """A herd condition as a C expression"""
        def atom(m):
            lhs, rhs = m.group(1), m.group(2)
            return "(%s == %s)" % (self.c_obs_value(lhs), self.c_rhs(rhs))
        out = re.sub(r"([\w]+(?::[\w]+)?)\s*=\s*(-?\w+(?::\w+)?)", atom, cond)
        out = out.replace("/\\", "&&").replace("\\/", "||").replace("~", "!")
        out = re.sub(r"\btrue\b", "1", out)
        out = re.sub(r"\bfalse\b", "0", out)
        return out

    def c_rhs(self, v):
        if re.fullmatch(r"-?\d+", v):
            return "(intptr_t) %s" % v
        if re.fullmatch(r"\d+:\w+", v):
            return self.c_obs_value(v)
        self.note_loc(v)
        return "(intptr_t) &__lk_%s" % v

    def c_obs_value(self, o):
        m = re.fullmatch(r"(\d+):([A-Za-z_]\w*)", o)
        if m:
            return "(intptr_t) __P%s_%s" % (m.group(1), m.group(2))
        return "(intptr_t) __lk_%s" % o

    # -- output -----------------------------------------------------------------

    def loc_type(self, name):
        t = self.locs.get(name)
        return t if t else "intptr_t"

    def c_init(self, val, typ):
        if val[0] == "int":
            if typ == "atomic_t":
                return "ATOMIC_INIT(%d)" % val[1]
            return str(val[1])
        self.note_loc(val[1])
        if "*" in typ:
            return "(%s) &__lk_%s" % (typ, val[1])
        return "(%s) &__lk_%s" % (typ, val[1])

    def translate(self):
        self.parse_params()
        self.parse_init()
        bodies = [(tid, self.rewrite_body(tid, body)) for tid, _, body in self.lt.threads]
        self.parse_final()

        for name, typ in self.locs.items():
            if typ in ("spinlock_t", "atomic_t") or (typ and "struct" in typ):
                if typ != "spinlock_t" and typ != "atomic_t":
                    raise Unsupported("location type %s" % typ)
        locs = sorted(self.locs)
        addr_code = {n: ADDR_BASE - k for k, n in enumerate(locs)}

        out = []
        out.append("/* Generated by scripts/lkmm/litmus2c.py from C litmus test %s */" %
                   self.lt.name)
        out.append('#include "%s"' % self.header)
        out.append("#include <assert.h>")
        out.append("#include <stdlib.h>")
        out.append("")
        # locations: declare all first (initializers may take addresses)
        for n in locs:
            out.append("extern %s __lk_%s;" % (self.loc_type(n), n))
        for n in locs:
            typ = self.loc_type(n)
            if typ == "spinlock_t":
                init = "PTHREAD_MUTEX_INITIALIZER"
            elif n in self.loc_init:
                init = self.c_init(self.loc_init[n], typ)
            elif typ == "atomic_t":
                init = "ATOMIC_INIT(0)"
            else:
                init = None
            out.append("%s __lk_%s%s;" % (typ, n, " = " + init if init else ""))
        out.append("")
        for (tid, r), typ in sorted(self.regs.items()):
            init = self.reg_init.get((tid, r))
            out.append("%s __P%d_%s%s;" % (typ, tid, r,
                                            " = " + self.c_init(init, typ) if init else ""))
        out.append("volatile intptr_t __lk_obs;")
        out.append("")
        for tid, body in bodies:
            regs = sorted(r for (t, r) in self.regs if t == tid)
            if not self.local_regs:
                for r in regs:
                    out.append("#define %s __P%d_%s" % (r, tid, r))
            out.append("void *P%d(void *__lk_arg)" % tid)
            out.append("{")
            if self.local_regs:
                for r in regs:
                    out.append("\t%s %s = __P%d_%s;" % (self.regs[(tid, r)], r, tid, r))
            for name, base in self.thread_params[tid]:
                out.append("\t%s *%s = (%s *) &__lk_%s;" % (base, name, base, name))
            out.append(body.rstrip())
            if self.local_regs:
                for r in regs:
                    out.append("\t__P%d_%s = %s;" % (tid, r, r))
            out.append("\treturn NULL;")
            out.append("}")
            if not self.local_regs:
                for r in regs:
                    out.append("#undef %s" % r)
            out.append("")

        # encode a value: an address of a location becomes its sentinel
        out.append("static intptr_t __lk_enc(intptr_t v)")
        out.append("{")
        for n in locs:
            out.append("\tif (v == (intptr_t) &__lk_%s)" % n)
            out.append("\t\treturn %d;" % addr_code[n])
        out.append("\treturn v;")
        out.append("}")
        out.append("")
        n = len(self.lt.threads)
        out.append("int main()")
        out.append("{")
        out.append("\tpthread_t t[%d];" % n)
        out.append("")
        for tid in range(n):
            out.append("\tif (pthread_create(&t[%d], NULL, P%d, NULL))" % (tid, tid))
            out.append("\t\tabort();")
        for tid in range(n):
            out.append("\tif (pthread_join(t[%d], NULL))" % tid)
            out.append("\t\tabort();")
        if self.filter:
            out.append("\t__VERIFIER_assume(%s);" % self.c_cond(self.filter))
        out.append('\t__VERIFIER_output("%s");' % OUTPUT_MARK)
        for o in self.observables:
            out.append("\t__lk_obs = __lk_enc(%s);" % self.c_obs_value(o))
        out.append("\treturn 0;")
        out.append("}")

        meta = {
            "name": self.lt.name,
            "expected": self.lt.expected,
            "threads": n,
            "quantifier": self.quantifier,
            "condition": self.condition,
            "observables": self.observables,
            "addresses": {str(v): k for k, v in addr_code.items()},
            "output_mark": OUTPUT_MARK,
        }
        return "\n".join(out) + "\n", meta


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("litmus")
    ap.add_argument("-o", "--output", default="-")
    ap.add_argument("--meta")
    ap.add_argument("--header", default="lkmm-c11.h",
                    help="header providing the kernel primitives")
    ap.add_argument("--local-regs", action="store_true",
                    help="registers as locals, copied to their globals at the end")
    ap.add_argument("--srcu", action="store_true",
                    help="translate SRCU calls (the header provides them: -lkmm)")
    args = ap.parse_args()
    try:
        text = open(args.litmus, errors="replace").read()
        src, meta = Translator(Litmus(text), args.header, args.srcu, args.local_regs).translate()
    except Unsupported as e:
        print("unsupported: %s" % e)
        sys.exit(3)
    if args.output == "-":
        sys.stdout.write(src)
    else:
        with open(args.output, "w") as f:
            f.write(src)
    if args.meta:
        with open(args.meta, "w") as f:
            json.dump(meta, f, indent=1)


if __name__ == "__main__":
    main()
