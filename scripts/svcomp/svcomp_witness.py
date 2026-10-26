#
# This program is dual-licensed under the Apache License 2.0 and the MIT License.
# You may choose to use, distribute, or modify this software under either license.

"""Violation witnesses for SV-COMP, from the error graph GenMC prints.

GenMC prints the graph of the execution that shows the error: each thread's
events, as "(t, i): KIND ... file:line", reads with the write they read
from, and thread creations as "THREAD_CREATE [thread N]". svcomp-run.py
runs GenMC so that these lines are the task's own (-library-dir: the code
the runner adds is reported at the task's line that called it).

A witness here gives the thread creations, in an order the execution has,
and the event(s) of the error: the call to reach_error() (unreach-call), the
two accesses of a race (no-data-race), the invalid access (valid-memsafety).
The validator finds the rest of the interleaving. Two formats:
  - GraphML 1.0, with threadId/createThread (SV-COMP 2026's format for
    violation witnesses in ConcurrencySafety);
  - YAML 2.2, whose waypoints carry thread_id and whose final segment holds
    both accesses of a race (the format's first version for concurrency).
"""
import datetime
import hashlib
import heapq
import re
import uuid
import xml.etree.ElementTree as ET

import yaml

HELPER_THREADS = {"__svc_chooser"}

EVENT = re.compile(r"^\s*\((\d+), (\d+)\): (.*?)\s*$")
HEADER = re.compile(r"^<(-?\d+), (\d+)>\s*(\S*?):\s*$")
LOC = re.compile(r"\s(\S+):(\d+)$")


def parse_error_graph(out):
    """The first graph GenMC prints after its error message: threads (their
    function), events (text, file, line) and the error's event(s)."""
    lines = out.splitlines()
    # (after the error: a warning, e.g. of unfreed memory, prints its graph
    # too; the program's output may precede the message on its line)
    err = next((i for i, l in enumerate(lines) if re.search(r"Error: ", l)), None)
    if err is None:
        return None
    start = next((i for i, l in enumerate(lines) if i > err and
                  re.match(r"Event \(\d+, \d+\) (conflicts with event \(\d+, \d+\) )?in graph:", l)),
                 None)
    if start is None:
        return None
    m = re.match(r"Event \((\d+), (\d+)\)(?: conflicts with event \((\d+), (\d+)\))?", lines[start])
    errors = [(int(m.group(1)), int(m.group(2)))]
    if m.group(3):
        errors.append((int(m.group(3)), int(m.group(4))))
    threads, events = {}, {}
    tid = None
    for l in lines[start + 1:]:
        if l.startswith(("Coherence:", "Trace to")) or not l.strip():
            break
        h = HEADER.match(l)
        if h:
            tid = int(h.group(2))
            threads[tid] = h.group(3) or "main"
            continue
        e = EVENT.match(l)
        if e and tid is not None:
            text = e.group(3)
            loc = LOC.search(text)
            events[(int(e.group(1)), int(e.group(2)))] = {
                "text": text,
                "file": loc.group(1) if loc else None,
                "line": int(loc.group(2)) if loc else None,
            }
    return {"threads": threads, "events": events, "errors": errors}


def linearize(graph):
    """The events in an order of the execution: program order, reads after
    the writes they read from, a thread's events after its creation, a join
    after the joined thread's events"""
    events = graph["events"]
    preds = {e: set() for e in events}
    by_thread = {}
    for t, i in sorted(events):
        by_thread.setdefault(t, []).append((t, i))
    for evs in by_thread.values():
        for a, b in zip(evs, evs[1:]):
            preds[b].add(a)
    for e, ev in events.items():
        rf = re.search(r"\[\((\d+), (\d+)\)\]", ev["text"])
        if rf and (int(rf.group(1)), int(rf.group(2))) in events:
            preds[e].add((int(rf.group(1)), int(rf.group(2))))
        c = re.match(r"THREAD_(CREATE|JOIN) \[thread (\d+)\]", ev["text"])
        if c and int(c.group(2)) in by_thread:
            child = by_thread[int(c.group(2))]
            if c.group(1) == "CREATE":
                preds[child[0]].add(e)
            else:
                preds[e].add(child[-1])
    order, ready = [], [e for e in events if not preds[e]]
    heapq.heapify(ready)
    succs = {e: [] for e in events}
    for e, ps in preds.items():
        for p in ps:
            succs[p].append(e)
    pending = {e: len(ps) for e, ps in preds.items()}
    while ready:
        e = heapq.heappop(ready)
        order.append(e)
        for s in succs[e]:
            pending[s] -= 1
            if not pending[s]:
                heapq.heappush(ready, s)
    return order


def task_events(graph, filename):
    """The witness's view of the execution: the thread creations (creator,
    created thread, the created thread's function, line), in an order the
    execution has, and the error events (thread, line), with threads named
    by the witness's numbering (main is 0, the k-th creation creates k).
    None if an event of the error is not at a line of the task."""
    ids = {0: 0}
    creations = []
    for e in linearize(graph):
        ev = graph["events"][e]
        c = re.match(r"THREAD_CREATE \[thread (\d+)\]", ev["text"])
        if not c:
            continue
        child = int(c.group(1))
        fn = graph["threads"].get(child, "")
        if fn in HELPER_THREADS or e[0] not in ids or ev["file"] != filename:
            continue
        ids[child] = len(creations) + 1
        creations.append({"thread": ids[e[0]], "creates": ids[child], "function": fn,
                          "line": ev["line"]})
    targets = []
    for e in graph["errors"]:
        ev = graph["events"].get(e)
        if not ev or ev["file"] != filename or e[0] not in ids:
            return None
        targets.append({"thread": ids[e[0]], "line": ev["line"],
                        "function": graph["threads"].get(e[0], "main"),
                        "reach_error": ev["text"].startswith("ERROR")})
    return {"creations": creations, "targets": targets}


def call_close_paren(source, line, fn):
    """(line, column) of the parenthesis that closes the call of FN that
    starts at LINE (1-based; the call may span lines)"""
    lines = source.splitlines()
    m = re.search(r"\b" + re.escape(fn) + r"\s*\(", lines[line - 1])
    if not m:
        return None
    depth = 0
    for ln in range(line - 1, len(lines)):
        start = m.end() - 1 if ln == line - 1 else 0
        for col in range(start, len(lines[ln])):
            ch = lines[ln][col]
            if ch == "(":
                depth += 1
            elif ch == ")":
                depth -= 1
                if depth == 0:
                    return ln + 1, col + 1
    return None


def target_column(source, line, reach_error):
    """The column of the innermost statement or full expression at LINE:
    the call of reach_error(), or what follows a label, an opening brace, a
    controlling keyword or return. None to leave it to the validator (the
    leftmost suitable position)"""
    text = source.splitlines()[line - 1]
    if reach_error:
        m = re.search(r"\breach_error\s*\(", text)
        return m.start() + 1 if m else None
    pos = len(text) - len(text.lstrip())
    while True:
        rest = text[pos:]
        m = (re.match(r"(?!default\b)[A-Za-z_]\w*\s*:(?!:)\s*", rest)
             or re.match(r"[{}]\s*", rest) or re.match(r"else\b\s*", rest)
             or re.match(r"(if|while|switch)\s*\(\s*", rest) or re.match(r"return\b\s*", rest))
        if not m or not m.group(0):
            break
        pos += len(m.group(0))
    if text.lstrip().startswith(("for", "do")) or pos >= len(text):
        return None
    return pos + 1


def sha256(path):
    with open(path, "rb") as f:
        return hashlib.sha256(f.read()).hexdigest()


def metadata(task_file, digest, spec, data_model, producer):
    return {
        "format_version": "2.2",
        "uuid": str(uuid.uuid4()),
        "creation_time": datetime.datetime.now(datetime.timezone.utc).isoformat(timespec="seconds"),
        "producer": {"name": "GenMC", "version": producer},
        "task": {
            "input_files": [task_file],
            "input_file_hashes": {task_file: digest},
            "specification": spec,
            "data_model": data_model,
            "language": "C",
        },
    }


def yaml_witness(model, task_file, source, spec, data_model, producer):
    """A violation_sequence (format 2.2): a segment per thread creation (a
    function_enter waypoint at the call of pthread_create, by the creating
    thread) and a final segment with the error's target waypoint(s)"""
    content = []
    for c in model["creations"]:
        loc = call_close_paren(source, c["line"], "pthread_create")
        if not loc:
            return None
        content.append({"segment": [{"waypoint": {
            "type": "function_enter", "action": "follow", "thread_id": c["thread"],
            "location": {"file_name": task_file, "line": loc[0], "column": loc[1]}}}]})
    final = []
    for t in model["targets"]:
        location = {"file_name": task_file, "line": t["line"]}
        col = target_column(source, t["line"], t["reach_error"])
        if col:
            location["column"] = col
        location["function"] = t["function"]
        final.append({"waypoint": {"type": "target", "action": "follow",
                                   "thread_id": t["thread"], "location": location}})
    content.append({"segment": final})
    entry = {"entry_type": "violation_sequence",
             "metadata": metadata(task_file, sha256(task_file), spec, data_model, producer),
             "content": content}
    return yaml.safe_dump([entry], sort_keys=False)


def graphml_witness(model, task_file, source, spec, data_model, producer):
    """An automaton (format 1.0) from the entry node through the thread
    creations (createThread, then the created thread's enterFunction) and the
    error's event(s) to the violation node"""
    ns = "http://graphml.graphdrawing.org/xmlns"
    root = ET.Element("graphml", {"xmlns": ns,
                                  "xmlns:xsi": "http://www.w3.org/2001/XMLSchema-instance"})
    keys = [("violation", "isViolationNode", "boolean", "node"),
            ("entry", "isEntryNode", "boolean", "node"),
            ("witness-type", "witness-type", "string", "graph"),
            ("sourcecodelang", "sourcecodeLanguage", "string", "graph"),
            ("producer", "producer", "string", "graph"),
            ("specification", "specification", "string", "graph"),
            ("programfile", "programFile", "string", "graph"),
            ("programhash", "programHash", "string", "graph"),
            ("architecture", "architecture", "string", "graph"),
            ("creationtime", "creationTime", "string", "graph"),
            ("startline", "startline", "int", "edge"),
            ("endline", "endline", "int", "edge"),
            ("threadId", "threadId", "string", "edge"),
            ("createThread", "createThread", "string", "edge"),
            ("enterFunction", "enterFunction", "string", "edge")]
    for kid, name, typ, dom in keys:
        k = ET.SubElement(root, "key", {"id": kid, "attr.name": name, "attr.type": typ,
                                        "for": dom})
        if typ == "boolean":
            ET.SubElement(k, "default").text = "false"
    g = ET.SubElement(root, "graph", {"edgedefault": "directed"})
    meta = metadata(task_file, sha256(task_file), spec, data_model, producer)
    for key, val in [("witness-type", "violation_witness"), ("sourcecodelang", "C"),
                     ("producer", "GenMC " + producer), ("specification", spec),
                     ("programfile", task_file),
                     ("programhash", meta["task"]["input_file_hashes"][task_file]),
                     ("architecture", "32bit" if data_model == "ILP32" else "64bit"),
                     ("creationtime", meta["creation_time"])]:
        ET.SubElement(g, "data", {"key": key}).text = val

    nodes = [0]

    def node(**data):
        n = "N%d" % len(nodes)
        nodes.append(n)
        el = ET.SubElement(g, "node", {"id": n})
        for k, v in data.items():
            ET.SubElement(el, "data", {"key": k}).text = v
        return n

    def edge(src, dst, **data):
        el = ET.SubElement(g, "edge", {"source": src, "target": dst})
        for k, v in data.items():
            ET.SubElement(el, "data", {"key": k}).text = str(v)

    cur = node(entry="true")
    for c in model["creations"]:
        nxt = node()
        edge(cur, nxt, threadId=c["thread"], createThread=c["creates"],
             startline=c["line"], endline=c["line"])
        cur, nxt = nxt, node()
        edge(cur, nxt, threadId=c["creates"], enterFunction=c["function"])
        cur = nxt
    for i, t in enumerate(model["targets"]):
        last = i + 1 == len(model["targets"])
        nxt = node(violation="true") if last else node()
        edge(cur, nxt, threadId=t["thread"], startline=t["line"], endline=t["line"])
        cur = nxt
    ET.indent(root)
    return '<?xml version="1.0" encoding="UTF-8" standalone="no"?>\n' + \
        ET.tostring(root, encoding="unicode") + "\n"


def write_witnesses(out, compiled_name, task_file, spec, data_model, prefix):
    """Writes PREFIX.graphml and PREFIX.yml for GenMC's output OUT, or
    nothing if the error is not at the task's lines. Returns the paths"""
    graph = parse_error_graph(out)
    if not graph:
        return []
    model = task_events(graph, compiled_name)
    if not model:
        return []
    source = open(task_file, errors="replace").read()
    m = re.search(r"GenMC v(\S+)", out)
    producer = m.group(1) if m else "unknown"
    written = []
    for ext, fn in (("graphml", graphml_witness), ("yml", yaml_witness)):
        text = fn(model, task_file, source, spec, data_model, producer)
        if text:
            with open(prefix + "." + ext, "w") as f:
                f.write(text)
            written.append(prefix + "." + ext)
    return written
