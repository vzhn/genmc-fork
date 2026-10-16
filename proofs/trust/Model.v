(** * Memory models (Section 3.1)

    TruSt is parametric in a consistency predicate satisfying three
    assumptions: well-formedness, prefix-closedness, maximal
    extensibility. They are recorded here as a class; SC, TSO, RA and
    RC11 satisfy them (the paper's footnote 2). *)
From TruSt Require Export Graph.

(** [prefix_closed G E]: [E] is a porf-prefix-closed set of events of [G]. *)
Definition porf_closed (G : Graph) (keep : Event → Prop) : Prop :=
  ∀ e1 e2, keep e2 → inG G e1 → porf G e1 e2 → keep e1.

(** A write [w] is co-maximal among the events satisfying [E]. *)
Definition co_maximal (G : Graph) (E : Event → Prop) (w : Event) : Prop :=
  E w ∧ ¬ ∃ w', E w' ∧ co G w w'.

Class MemoryModel := {
  consistent : Graph → Prop;

  (** Well-formedness: consistency does not depend on the insertion
      order, and porf is acyclic in consistent graphs. *)
  cons_order_irrelevant : ∀ G G', gequiv G G' → consistent G → consistent G';
  cons_porf_acyclic : ∀ G, consistent G → ∀ e, ¬ porf G e e;

  (** Prefix-closedness: restricting to a porf-closed set of events keeps
      the graph consistent. *)
  cons_prefix_closed : ∀ G (keep : Event → Prop) `{∀ e, Decision (keep e)},
      consistent G → porf_closed G keep → keep EInit → consistent (restrict G keep);

  (** Maximal extensibility: a po-maximal event can be added co-maximally
      (writes) or reading from the co-maximal same-location write (reads)
      without breaking consistency. *)
  cons_max_ext_write : ∀ G a l, consistent G → ¬ inG G a → writes_loc a l → loc a = Some l →
      (∀ e, inG G e → ¬ po a e) →
      ∀ wp, co_maximal G (λ w, inG G w ∧ writes_loc w l) wp →
      consistent (setCO (add G a) wp a);
  cons_max_ext_read : ∀ G a l, consistent G → ¬ inG G a → is_read a → loc a = Some l →
      (∀ e, inG G e → ¬ po a e) →
      ∀ w, co_maximal G (λ w, inG G w ∧ writes_loc w l) w →
      consistent (setRF (add G a) a w);
}.
