/*
 * GenMC reports an error as soon as it finds one on a consistent graph. That
 * is sound only if the events a thread must add next cannot make the graph
 * inconsistent: otherwise the error is on a prefix no execution has. The
 * events here are the fences a thread adds at its end; under LKMM, the
 * rcu_read_unlock() that closes an open read-side critical section, which
 * orders the section's events against the grace periods (rcu-order, rb).
 *
 * For each model, `check` looks for a graph G' = G + NF, where G (with the
 * thread of NF ending inside the section NF closes, under LKMM) is consistent
 * and G' is not. A counterexample means that an error found inside an open
 * section must be checked with the section closed before it is reported
 * (openSectionsNeedCheck() in Execution/MemoryModel.hpp,
 * ExplorationCore::reportError()); none, within the scope, means a trailing
 * fence keeps graphs consistent.
 *
 * The C11 models are those of rmw-placement.als (Kater's
 * kat/{sc,tso,ra,rc11}-genmc.kat for relaxed, release and acquire accesses),
 * with fences as events of po only. LKMM is the RCU fragment of
 * kat/lkmm-genmc.kat as GenMC evaluates it (Models/LkmmModel.cpp,
 * lkmm::evaluate()): marked relaxed accesses, synchronize_rcu(),
 * rcu_read_lock() and rcu_read_unlock(); no dependencies, no release or
 * acquire, no plain accesses, no SRCU, no thread creation or joins. The
 * fixpoint rcu-order is unrolled three times, which is exact for the chains
 * of grace periods and sections a graph within the scope can have. IMM is
 * not modelled: it has no critical sections, and nothing here distinguishes
 * it from RC11.
 *
 * Run: scripts/check-alloy-proofs.sh (ctest formal-models).
 */
module opensection

sig Thread {}
sig Loc {}

abstract sig Event {
	thr: lone Thread,
	po: set Event,
}
abstract sig Access extends Event {
	loc: one Loc,
}
sig Rd extends Access {
	rf: one Wr,
}
sig Wr extends Access {
	co: set Wr,
}
/* The initial writes: no thread, one per location */
sig Init in Wr {}
/* Release writes and acquire reads (RA: all of them) */
sig Rel in Wr {}
sig Acq in Rd {}

/* Fences: under LKMM, synchronize_rcu(), rcu_read_lock() and
 * rcu_read_unlock(); under the C11 models, events of po only */
abstract sig Fence extends Event {}
sig Sync extends Fence {}
sig Lock extends Fence {}
sig Unlock extends Fence {}

/* The fence added last, at the end of its thread */
one sig NF in Fence {}

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
}

fact lastFence {
	no NF.po
}

/* --- the graph restricted to the events in P ------------------------------ */

fun poP[P: set Event]: Event -> Event { P <: po :> P }
fun rfP[P: set Event]: Event -> Event { P <: rf :> P }
fun coP[P: set Event]: Event -> Event { P <: co :> P }
fun frP[P: set Event]: Event -> Event { let r = rfP[P], c = coP[P] | (~r . c) - iden }
fun sameThr: Event -> Event { thr.~thr }
fun ext: Event -> Event { (Event -> Event) - thr.~thr }
fun ecoP[P: set Event]: Event -> Event { ^(rfP[P] + coP[P] + frP[P]) }

pred acyclic[r: Event -> Event] { no iden & ^r }

/* hb;eco is irreflexive (Kater's `coherence hb`) */
pred coherent[P: set Event, hb: Event -> Event] {
	let eco = ecoP[P] | no iden & (hb . eco)
	no iden & ecoP[P]
}

/* --- the C11 models (rmw-placement.als, without RMWs) ---------------------- */

/* sc-genmc.kat: hb = po | rf; acyclic po | rf | co | fr */
pred sc[P: set Event] {
	coherent[P, ^(poP[P] + rfP[P])] and acyclic[poP[P] + rfP[P] + coP[P] + frP[P]]
}

/* tso-genmc.kat: hb = po; tso = [R];po | po;[W] | rfe | mo | fr */
pred tso[P: set Event] {
	let tsoRel = (Rd <: poP[P]) + (poP[P] :> Wr) + (rfP[P] & ext) + coP[P] + frP[P] |
	coherent[P, ^(poP[P])] and acyclic[tsoRel]
}

/* ra-genmc.kat with every write release and every read acquire: hb = (po | rf)+ */
pred ra[P: set Event] {
	coherent[P, ^(poP[P] + rfP[P])]
}

/* rc11-genmc.kat without SC accesses and fences: hb = (po | sw)+, sw = [REL];rf;[ACQ] */
pred rc11[P: set Event] {
	let sw = Rel <: rfP[P] :> Acq |
	coherent[P, ^(poP[P] + sw)]
}

/* --- LKMM's RCU fragment --------------------------------------------------- */

/* The read-side critical sections of P: a lock and the unlock that closes
 * it, innermost first (the bell's fixpoint matching) */
fun between[P: set Event, a, b: Event]: set Event { P & a.po & po.b }
pred balanced[P: set Event, a, b: Event] {
	#(between[P, a, b] & Lock) = #(between[P, a, b] & Unlock)
}
fun rscsP[P: set Event]: Lock -> Unlock {
	{ l: P & Lock, u: P & Unlock | l -> u in po and balanced[P, l, u] and
		no u2: between[P, l, u] & Unlock | balanced[P, l, u2] }
}

/* GenMC rejects the other programs: an unlock without a lock, a grace period
 * inside a section (handleLkmmFence()) */
fact rcuWellFormed {
	all u: Unlock | one l: Lock | l -> u in rscsP[Event]
	all s: Sync, l: Lock | l -> s in po implies (some u: l.(rscsP[Event]) | u -> s in po)
}

/* gp = po ; [Sync] ; po?: the only fence here, hence strong-fence and fence */
fun gpP[P: set Event]: Event -> Event { (poP[P] :> (P & Sync)) . (poP[P] + iden) }

/* ppo = to-w | fence: (moi | fri) | gp (no dependencies, no other fences) */
fun ppoP[P: set Event]: Event -> Event { ((coP[P] + frP[P]) & sameThr) + gpP[P] }

/* cumul-fence = [M] ; rfe? ; strong-fence ; [M]
 * prop = [M] ; (moe | fre)? ; cumul-fence* ; [M] ; rfe? ; [M] */
fun cumulFenceP[P: set Event]: Event -> Event {
	let rfe = rfP[P] & ext | Access <: (((rfe + iden) . (gpP[P]))) :> Access
}
fun propP[P: set Event]: Event -> Event {
	let rfe = rfP[P] & ext, overwritee = (coP[P] + frP[P]) & ext |
	Access <: (((overwritee + iden) . *(cumulFenceP[P])) :> Access) . (rfe + iden) :> Access
}

/* hb = [M] ; (ppo | rfe | (prop \ id) & int) ; [M] */
fun hbP[P: set Event]: Event -> Event {
	let rfe = rfP[P] & ext, propInt = (propP[P] - iden) & sameThr |
	Access <: (ppoP[P] + rfe + propInt) :> Access
}

/* pb = prop ; strong-fence ; hb* ; [M] */
fun pbP[P: set Event]: Event -> Event { ((propP[P] . (gpP[P])) . *(hbP[P])) :> Access }

/* RCU's events: grace periods and the ends of critical sections */
fun rcuEventsP[P: set Event]: set Event {
	(P & Sync) + (rscsP[P]).Unlock + Lock.(rscsP[P])
}

/* po? ; hb* ; pb* from RCU's events; rcu-link = that ; prop ; po */
fun reachP[P: set Event]: Event -> Event {
	((rcuEventsP[P] <: (po + iden)) . *(hbP[P])) . *(pbP[P])
}
fun linkP[P: set Event]: Event -> Event { (reachP[P] . (propP[P])) . (poP[P]) }

/* rcu-order, the least fixpoint of
 *   rcu-gp | rcu-gp ; link ; rcu-rscsi | rcu-rscsi ; link ; rcu-gp
 *   | rcu-gp ; link ; rcu-order ; link ; rcu-rscsi
 *   | rcu-rscsi ; link ; rcu-order ; link ; rcu-gp
 *   | rcu-order ; link ; rcu-order
 * unrolled three times (exact for the chains a graph within the scope has) */
fun orderStep[P: set Event, order: Event -> Event]: Event -> Event {
	let gp = iden & (P & Sync) -> (P & Sync), rscsi = ~(rscsP[P]), link = linkP[P] |
	order + (((gp . link) . order) . link) . rscsi + (((rscsi . link) . order) . link) . gp +
		(order . link) . order
}
fun rcuOrderP[P: set Event]: Event -> Event {
	let gp = iden & (P & Sync) -> (P & Sync), rscsi = ~(rscsP[P]), link = linkP[P] |
	let order0 = gp + (gp . link) . rscsi + (rscsi . link) . gp |
	orderStep[P, orderStep[P, orderStep[P, order0]]]
}

/* rb = prop ; po ; rcu-order ; po? ; hb* ; pb* ; [M] is reflexive: some
 * access reaches an RCU event by prop ; po and comes back by po? ; hb* ; pb*
 * from one rcu-order puts after it (lkmm::evaluate()) */
pred rbReflexive[P: set Event] {
	let f = rcuEventsP[P], toRcu = propP[P] . (poP[P] :> f) |
	some a: P & Access | a in a.((toRcu . (rcuOrderP[P])) . (reachP[P]))
}

/* Coherence: acyclic (po-loc | rf | co | fr) */
pred lkmmCoherent[P: set Event] {
	acyclic[(poP[P] & loc.~loc) + rfP[P] + coP[P] + frP[P]]
}

pred lkmm[P: set Event] {
	lkmmCoherent[P] and acyclic[hbP[P]] and acyclic[pbP[P]] and not rbReflexive[P]
}

/* --- the lemma, per model ----------------------------------------------- */

assert closingKeepsSC { sc[Event - NF] implies sc[Event] }
assert closingKeepsTSO { tso[Event - NF] implies tso[Event] }
assert closingKeepsRA { ra[Event - NF] implies ra[Event] }
assert closingKeepsRC11 { rc11[Event - NF] implies rc11[Event] }
/* NF is the rcu_read_unlock() that closes a section open in G */
assert closingKeepsLKMM { (NF in Unlock and lkmm[Event - NF]) implies lkmm[Event] }

/* Not vacuous: each model has consistent graphs where the trailing fence
 * follows accesses of its thread; under LKMM, a graph with a grace period
 * and a section that NF closes, with accesses inside it */
pred fenceAfterAccesses { some a: Access | a -> NF in po }
pred openSection {
	NF in Unlock and some Sync and some l: Lock | l -> NF in rscsP[Event] and
		some a: Access | l -> a in po and a -> NF in po
}
run premiseSC { sc[Event - NF] and fenceAfterAccesses } for 7 but 3 Thread, 2 Loc
run premiseTSO { tso[Event - NF] and fenceAfterAccesses } for 7 but 3 Thread, 2 Loc
run premiseRA { ra[Event - NF] and fenceAfterAccesses } for 7 but 3 Thread, 2 Loc
run premiseRC11 { rc11[Event - NF] and fenceAfterAccesses } for 7 but 3 Thread, 2 Loc
run premiseLKMM { lkmm[Event - NF] and openSection } for 9 but 2 Thread, 2 Loc

check closingKeepsSC for 7 but 3 Thread, 2 Loc
check closingKeepsTSO for 7 but 3 Thread, 2 Loc
check closingKeepsRA for 7 but 3 Thread, 2 Loc
check closingKeepsRC11 for 7 but 3 Thread, 2 Loc
check closingKeepsLKMM for 9 but 2 Thread, 2 Loc
