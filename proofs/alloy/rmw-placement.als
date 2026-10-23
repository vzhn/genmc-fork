/*
 * When GenMC adds an RMW, it checks the read part as it adds it, and puts
 * the write part in coherence right after the write the read part reads
 * (atomicity), which need not be the last write of the location. Whether
 * that placement can make a consistent graph inconsistent depends on the
 * model: where it can, GenMC must check the graph again once the write part
 * is placed (ExplorationCore::isPlacementConsistent(), the per-model table
 * rmwPlacementNeedsCheck() in Execution/MemoryModel.hpp).
 *
 * For each model, `check` looks for a graph G' = G_r + W, where G_r (the
 * graph with the RMW's read part R, reading S, added last in its thread) is
 * consistent, W is R's write part, added right after R in po and right after
 * S in co, and G' is not consistent. A counterexample means the model needs
 * the check; none, within the scope, means the placement keeps graphs
 * consistent.
 *
 * The axioms are those of Kater's kat/{sc,tso,ra,rc11}-genmc.kat for
 * relaxed, release and acquire accesses to locations (no fences, no SC
 * accesses, no thread creation or joins), plus atomicity, which GenMC checks
 * outside the models (ExplorationCore::violatesAtomicity()). IMM and LKMM
 * are not modelled: the table checks them (LKMM because of a known cycle
 * through prop, IMM because nothing here shows that it need not).
 *
 * Run: scripts/check-alloy-proofs.sh (ctest formal-models).
 */
module rmwplacement

sig Thread {}
sig Loc {}

abstract sig Event {
	thr: lone Thread,
	loc: one Loc,
	po: set Event,
}
sig Rd extends Event {
	rf: one Wr,
}
sig Wr extends Event {
	co: set Wr,
}
/* The initial writes: no thread, one per location */
sig Init in Wr {}
/* Release writes and acquire reads (RA: all of them) */
sig Rel in Wr {}
sig Acq in Rd {}

one sig RMW {
	pairs: Rd -> Wr,
}

/* The RMW added last: its read part NR, then its write part NW */
one sig NR in Rd {}
one sig NW in Wr {}

fact wellFormed {
	all e: Event | no e.thr iff e in Init
	all x: Loc | one i: Init | i.loc = x
	/* po: a strict total order of each thread's events */
	po in thr.~thr
	no iden & po
	po.po in po
	all disj a, b: Event - Init | a.thr = b.thr implies (a -> b in po or b -> a in po)
	/* rf: from a write to the same location */
	all r: Rd | r.rf.loc = r.loc
	/* co: a strict total order of the writes of each location, initial first */
	co in loc.~loc
	no iden & co
	co.co in co
	all disj a, b: Wr | a.loc = b.loc implies (a -> b in co or b -> a in co)
	all i: Init, w: Wr - Init | i.loc = w.loc implies i -> w in co
	/* rmw: a read and its po-next write, same location */
	all r: Rd, w: Wr | r -> w in RMW.pairs implies
		(r.loc = w.loc and r -> w in po and no (r.po & w.~po))
	all r: Rd | lone r.(RMW.pairs)
	all w: Wr | lone (RMW.pairs).w
	no Init & (Rd.(RMW.pairs))
}

fact newRMW {
	NR -> NW in RMW.pairs
	/* added last in their thread, NW not read yet, NW right after NR's rf */
	no NW.po
	NR.po = NW
	no rf.NW
	NR.rf -> NW in co
	no w: Wr | NR.rf -> w in co and w -> NW in co
}

/* --- the graph restricted to the events in P ------------------------------ */

fun poP[P: set Event]: Event -> Event { P <: po :> P }
fun rfP[P: set Event]: Event -> Event { P <: rf :> P }
fun coP[P: set Event]: Event -> Event { P <: co :> P }
fun frP[P: set Event]: Event -> Event { let r = rfP[P], c = coP[P] | (~r . c) - iden }
fun rmwP[P: set Event]: Event -> Event { P <: RMW.pairs :> P }
fun ext: Event -> Event { (Event -> Event) - thr.~thr }
fun ecoP[P: set Event]: Event -> Event { ^(rfP[P] + coP[P] + frP[P]) }

pred acyclic[r: Event -> Event] { no iden & ^r }

/* hb;eco is irreflexive (Kater's `coherence hb`) */
pred coherent[P: set Event, hb: Event -> Event] {
	let eco = ecoP[P] | no iden & (hb . eco)
	no iden & ecoP[P]
}
/* No write between an RMW's read and write in coherence */
pred atomic[P: set Event] {
	let rmw = rmwP[P], fre = frP[P] & ext, coe = coP[P] & ext | no rmw & (fre . coe)
}

/* --- the models --------------------------------------------------------- */

/* sc-genmc.kat: hb = po | [ATOM];rf;[ATOM] (all accesses here are atomic) */
pred sc[P: set Event] {
	let hb = ^(poP[P] + rfP[P]) |
	coherent[P, hb] and atomic[P] and acyclic[poP[P] + rfP[P] + coP[P] + frP[P]]
}

/* tso-genmc.kat: hb = po; tso = [R|UW];po | po;[W] | rfe | mo | fr */
pred tso[P: set Event] {
	let rmw = rmwP[P] | let UW = Rd.rmw |
	let tsoRel = ((Rd + UW) <: poP[P]) + (poP[P] :> Wr) + (rfP[P] & ext) + coP[P] + frP[P] |
	coherent[P, ^(poP[P])] and atomic[P] and acyclic[tsoRel]
}

/* ra-genmc.kat with every write release and every read acquire:
 * hb = (po | rf)+ */
pred ra[P: set Event] {
	coherent[P, ^(poP[P] + rfP[P])] and atomic[P]
}

/* rc11-genmc.kat without SC accesses and fences: hb = (po | sw)+, with
 * sw = [REL];(rf;rmw)*;rf;[ACQ] */
pred rc11[P: set Event] {
	let r = rfP[P], rmw = rmwP[P] | let sw = Rel <: (*(r . rmw) . r) :> Acq |
	coherent[P, ^(poP[P] + sw)] and atomic[P]
}

/* --- the lemma, per model ----------------------------------------------- */

assert placementKeepsSC { sc[Event - NW] implies sc[Event] }
assert placementKeepsTSO { tso[Event - NW] implies tso[Event] }
assert placementKeepsRA { ra[Event - NW] implies ra[Event] }
assert placementKeepsRC11 { rc11[Event - NW] implies rc11[Event] }

/* Not vacuous: each model has graphs where the premise holds and the RMW's
 * write is not the last write of its location (the case at stake) */
pred notLast { some w: Wr | NW -> w in co }
run premiseSC { sc[Event - NW] and notLast } for 7 but 3 Thread, 2 Loc
run premiseTSO { tso[Event - NW] and notLast } for 7 but 3 Thread, 2 Loc
run premiseRA { ra[Event - NW] and notLast } for 7 but 3 Thread, 2 Loc
run premiseRC11 { rc11[Event - NW] and notLast } for 7 but 3 Thread, 2 Loc

check placementKeepsSC for 7 but 3 Thread, 2 Loc
check placementKeepsTSO for 7 but 3 Thread, 2 Loc
check placementKeepsRA for 7 but 3 Thread, 2 Loc
check placementKeepsRC11 for 7 but 3 Thread, 2 Loc
