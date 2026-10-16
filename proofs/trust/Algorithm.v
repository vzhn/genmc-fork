(** * The TruSt algorithm (Algorithm 1, recursive version)

    We model [Visit] as the inductive predicate [visit P G] meaning
    "[Verify(P)] calls [Visit(P, G)] somewhere during its exploration".
    Each constructor is one line of Algorithm 1. Termination is not
    needed for this relational presentation; soundness and completeness
    are statements about [visit].

    The consistency check of line 4 is the hypothesis [consistent G] of
    every constructor that continues from [G]. *)
From Stdlib Require Import Classical ClassicalEpsilon.
From TruSt Require Export Program Model.

(** The algorithm restricts graphs by sets defined through [porf]; we do not
    build a decision procedure for [porf] here and treat the specification
    classically. *)
Global Instance classical_decision (P : Prop) : Decision P | 0 :=
  excluded_middle_informative P.

Section Algorithm.
  Context `{MemoryModel}.
  Variable P : Program.

  (** [Deleted] of line 16: events added after [r] that are not
      porf-before the revisiting write [a]. Note that [a] itself is not in
      [G] here; the porf-prefix of [a] is computed in [add G a]. *)
  Definition deleted (G : Graph) (r a e : Event) : Prop :=
    inG G e ∧ added_sbefore G r e ∧ ¬ porf (add G a) e a.

  (** [Previous_G(e, w)] of line 24. *)
  Definition previous (G : Graph) (e w : Event) (e' : Event) : Prop :=
    inG G e' ∧ (added_before G e' e ∨ porf G e' w).

  (** [IsMaximallyAdded(G, e, w)] (lines 23-27). *)
  Definition is_maximally_added (G : Graph) (e w : Event) : Prop :=
    (¬ ∃ r, previous G e w r ∧ rf G r = Some e) ∧
    ∃ e', (if decide (is_read e) then rf G e = Some e' else e' = e) ∧
          previous G e w e' ∧
          ¬ ∃ w', previous G e w w' ∧ co G e' w'.

  Inductive visit : Graph → Prop :=
  | visit_init :
      visit G0
  (** Line 10-12: a read reads from any same-location write of [G]. *)
  | visit_read G a l w :
      visit G → consistent G → next P G = Some a →
      is_read a → loc a = Some l →
      inG G w → writes_loc w l →
      visit (setRF (add G a) a w)
  (** Line 13-14 and 21-22: a write is placed after any same-location write. *)
  | visit_write_co G a l wp :
      visit G → consistent G → next P G = Some a →
      is_write a → loc a = Some l →
      inG G wp → writes_loc wp l →
      visit (setCO (add G a) wp a)
  (** Line 15-18: a write backward-revisits a same-location read [r] that
      is not porf-before it, provided every deleted event and [r] itself
      were added maximally. *)
  | visit_write_rev G a l r wp :
      visit G → consistent G → next P G = Some a →
      is_write a → loc a = Some l →
      inG G r → is_read r → loc r = Some l → ¬ porf (add G a) r a →
      (∀ e, deleted G r a e ∨ e = r → is_maximally_added (add G a) e a) →
      let G' := setRF (add (restrict G (λ e, ¬ deleted G r a e)) a) r a in
      inG G' wp → writes_loc wp l →
      visit (setCO G' wp a)
  (** Line 19-20: any other event (a fence) is just added. *)
  | visit_other G a :
      visit G → consistent G → next P G = Some a →
      ¬ is_read a → ¬ is_write a → ¬ is_error a →
      visit (add G a).

  (** A graph the algorithm *generates*: visited, consistent, and full
      (line 6-7). *)
  Definition generated (G : Graph) : Prop :=
    visit G ∧ consistent G ∧ full P G.

  (** The algorithm reports an error when it visits a consistent graph
      whose next event is an error event (line 8-9). *)
  Definition reports_error : Prop :=
    ∃ G a, visit G ∧ consistent G ∧ next P G = Some a ∧ is_error a.
End Algorithm.
