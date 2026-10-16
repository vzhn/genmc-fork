# Mechanization of TruSt

A Rocq (Coq) formalization of the TruSt algorithm from *Truly Stateless,
Optimal Dynamic Partial Order Reduction* (Kokologiannakis, Marmanis,
Gladstein, Vafeiadis, POPL 2022), which is the exploration algorithm at the
core of GenMC (`CHANGELOG.md`, 0.7: "Core algorithm now follows TruSt").

The goal is a verified reference model of the algorithm against which the
C++ implementation in `genmc/genmc/Verification/GenMCDriver.cpp` can be
checked by differential testing, and eventually proofs of the three
theorems of Section 4.3: soundness, completeness, optimality.

## Building

Needs the Rocq toolchain from the `iris` opam switch (Rocq 9.3, stdpp 1.13):

```bash
make            # builds trust/*.v
make clean
```

## Structure (`trust/`)

| File | Content | Paper |
|---|---|---|
| `Event.v` | events, labels, program order `po` | Def. 3.1 |
| `Graph.v` | execution graphs with insertion order, `rf`, `co`; `add`, `setRF`, `setCO`, `restrict`; `graph_wf` | Def. 3.2 |
| `Program.v` | data-deterministic programs, `next_P`, `hist`, `execution_of` | §4.1 |
| `Model.v` | memory model as a class: consistency plus the three assumptions (well-formedness, prefix-closedness, maximal extensibility) | §3.1 |
| `Lemmas.v`, `Order.v`, `WF.v` | list lemmas, the insertion order as a total order, well-formedness preservation | |
| `Algorithm.v` | Algorithm 1 as the inductive predicate `visit P G` ("Visit(P,G) is called"), `deleted`, `is_maximally_added`, `generated` | Alg. 1 |
| `Soundness.v` | **proved:** every visited graph is an execution of P; every generated graph is a consistent full execution | §4.3 |
| `Completeness.v` | statements of completeness and optimality, not yet proved | Thm. 4.1, 4.2 |

## What is proved

`soundness : generated P G → consistent G ∧ full P G ∧ execution_of P G`

The content is `visit_execution`: the backward-revisit step restricts the
graph to the events not deleted and the restriction stays an execution of
the program. The key lemma is `keep_rf`: a surviving read never reads from
a deleted write, because the first clause of `IsMaximallyAdded` (line 25 of
Algorithm 1) forbids deleting a write that some earlier event reads from.
No memory-model assumption is needed for soundness; consistency is checked
by the algorithm itself.

## Modeling choices

- The insertion order `≤_G` is the position in the graph's event list.
- Programs are data-deterministic: a thread's next label is a function of
  the values its earlier reads returned (`Program.step`). `<next` is thread
  order, which includes po.
- RMW (exclusive) events, blocking, and `--mode=random` are not modeled.
- Restriction by a `porf`-defined set is specified classically
  (`classical_decision`): the development uses `Classical` and
  `ClassicalEpsilon` from Stdlib, listed by `Print Assumptions soundness`.
- The algorithm is relational (`visit`), so termination is a separate
  statement, as in the paper.

## Next steps

1. Completeness (Theorem 4.1): define `Prev` (Algorithm 3) and show every
   consistent full execution is reached by a sequence of backward steps.
2. Optimality (Theorem 4.2): refine `visit` into an explicit call tree and
   show no configuration is reached twice.
3. Extract `next`/`visit` as a reference checker for small programs and
   compare its set of explored graphs against `genmc -print-exec-graphs`.
