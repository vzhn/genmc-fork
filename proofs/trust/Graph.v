(** * Execution graphs (Definition 3.2)

    A graph keeps its events as a list in insertion order ([EInit] first),
    which gives the total order [≤_G] of the paper for free. [rf] maps a
    read to the write it reads from; [co] is the coherence order. *)
From TruSt Require Export Event.

Record Graph := mkG {
  evs : list Event;                 (* in insertion order; [EInit] first *)
  rf  : Event → option Event;       (* reads-from, defined on reads *)
  co  : Event → Event → Prop;       (* coherence order *)
}.

Definition inG (G : Graph) (e : Event) : Prop := e ∈ evs G.

(** [added_before G e1 e2]: [e1 ≤_G e2]. *)
Definition added_before (G : Graph) (e1 e2 : Event) : Prop :=
  (e1 = e2 ∧ inG G e1) ∨ ∃ l1 l2 l3, evs G = l1 ++ e1 :: l2 ++ e2 :: l3.
Definition added_sbefore (G : Graph) (e1 e2 : Event) : Prop :=
  added_before G e1 e2 ∧ e1 ≠ e2.

(** Same-location writes of [G]. *)
Definition writes_of (G : Graph) (l : Loc) : list Event :=
  filter (λ w, writes_loc w l) (evs G).
Definition reads_of (G : Graph) (l : Loc) : list Event :=
  filter (λ r, loc r = Some l ∧ is_read r) (evs G).

(** Reads-from as a relation (write first), restricted to the graph. *)
Definition rf_rel (G : Graph) (w r : Event) : Prop :=
  inG G r ∧ is_read r ∧ rf G r = Some w.

(** [porf]: transitive closure of program order and reads-from among
    the graph's events. *)
Definition porf_step (G : Graph) (e1 e2 : Event) : Prop :=
  (inG G e1 ∧ inG G e2 ∧ po e1 e2) ∨ rf_rel G e1 e2.
Definition porf (G : Graph) : Event → Event → Prop := tc (porf_step G).

(** The value a read obtains: the value written by its [rf] source. *)
Definition rval (G : Graph) (r : Event) : option Val :=
  rf G r ≫= wval.

(** Graph operations used by the algorithm. *)

(** Add a po-maximal event at the end of the insertion order. *)
Definition add (G : Graph) (a : Event) : Graph :=
  mkG (evs G ++ [a]) (rf G) (co G).

(** [SetRF(G, r, w)]: make [r] read from [w]. *)
Definition setRF (G : Graph) (r w : Event) : Graph :=
  mkG (evs G) (λ e, if decide (e = r) then Some w else rf G e) (co G).

(** [SetCO(G, wp, a)]: insert [a] in [co] immediately after [wp]:
    everything [co]-before-or-equal to [wp] is before [a], and everything
    [co]-after [wp] is after [a]. *)
Definition setCO (G : Graph) (wp a : Event) : Graph :=
  mkG (evs G) (rf G)
      (λ x y, co G x y
              ∨ (y = a ∧ (x = wp ∨ co G x wp))
              ∨ (x = a ∧ co G wp y)).

(** [G|_E]: restriction to the events satisfying [keep]. *)
Definition restrict (G : Graph) (keep : Event → Prop) `{∀ e, Decision (keep e)} : Graph :=
  mkG (filter keep (evs G))
      (λ e, if decide (keep e) then rf G e else None)
      (λ x y, keep x ∧ keep y ∧ co G x y).

(** [G1 ≈ G2]: equal up to the insertion order. *)
Definition gequiv (G1 G2 : Graph) : Prop :=
  (∀ e, inG G1 e ↔ inG G2 e) ∧
  (∀ r, inG G1 r → rf G1 r = rf G2 r) ∧
  (∀ x y, inG G1 x → inG G1 y → (co G1 x y ↔ co G2 x y)).

(** ** Basic well-formedness, independent of any memory model *)

Definition thread_events (G : Graph) (t : Tid) : list Event :=
  filter (λ e, tid e = Some t) (evs G).

(** Number of events of thread [t] in [G]. *)
Definition count (G : Graph) (t : Tid) : nat := length (thread_events G t).

Record graph_wf (G : Graph) : Prop := {
  wf_init : head (evs G) = Some EInit;
  wf_nodup : NoDup (evs G);
  wf_ids : ∀ t i l1 l2, inG G (EEv t i l1) → inG G (EEv t i l2) → l1 = l2;
  (* a thread's serial numbers are exactly [0 .. count-1], in insertion order *)
  wf_idx : ∀ t i l, inG G (EEv t i l) → i < count G t;
  wf_contig : ∀ t j, j < count G t → ∃ l, inG G (EEv t j l);
  wf_po_order : ∀ e1 e2, inG G e1 → inG G e2 → po e1 e2 → added_before G e1 e2;
  (* rf is total on reads, points into the graph, and is same-location *)
  wf_rf : ∀ r l, inG G r → loc r = Some l → is_read r →
            ∃ w, rf G r = Some w ∧ inG G w ∧ writes_loc w l;
  wf_rf_none : ∀ e, ¬ is_read e → rf G e = None;
}.

Lemma inG_add G a e : inG (add G a) e ↔ inG G e ∨ e = a.
Proof. unfold inG, add; simpl. rewrite elem_of_app, list_elem_of_singleton. tauto. Qed.

Lemma inG_setRF G r w e : inG (setRF G r w) e ↔ inG G e.
Proof. done. Qed.

Lemma inG_setCO G wp a e : inG (setCO G wp a) e ↔ inG G e.
Proof. done. Qed.

Lemma inG_restrict G keep `{∀ e, Decision (keep e)} e :
  inG (restrict G keep) e ↔ inG G e ∧ keep e.
Proof. unfold inG, restrict; simpl. rewrite list_elem_of_filter. tauto. Qed.
