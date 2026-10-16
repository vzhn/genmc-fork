(** * Well-formedness is preserved by the graph operations *)
From TruSt Require Export Order.

(** ** A fresh next event *)

Section Add.
  Context (G : Graph) (t : Tid) (l : Lab).
  Let a := EEv t (count G t) l.
  Context (Hwf : graph_wf G).

  Lemma fresh : ¬ inG G a.
  Proof. intros H. apply (wf_idx _ Hwf) in H. lia. Qed.

  Lemma count_add_a t' : count (add G a) t' = if decide (t' = t) then S (count G t) else count G t'.
  Proof.
    destruct (decide (t' = t)) as [->|].
    - by apply count_add_same.
    - apply count_add_other. simpl. congruence.
  Qed.

  Lemma ev_at_add_old t' k :
    (t' ≠ t ∨ k ≠ count G t) → ev_at (add G a) t' k = ev_at G t' k.
  Proof.
    intros Hk. unfold ev_at, add; simpl.
    destruct (list_find _ (evs G)) as [[i x]|] eqn:Hf.
    - by rewrite (list_find_app_l _ _ _ _ _ Hf).
    - rewrite (list_find_app_r _ _ _ Hf). simpl.
      rewrite decide_False; [done|]. simpl. intros [? ?]. simplify_eq. lia.
  Qed.

  Lemma hist_add_old t' i :
    (t' ≠ t ∨ i ≤ count G t) → hist (add G a) t' i = hist G t' i.
  Proof.
    intros Hi. apply hist_ext.
    - intros k Hk. apply ev_at_add_old. destruct Hi; [auto|right; lia].
    - done.
  Qed.

  Lemma wf_add_core :
    head (evs (add G a)) = Some EInit ∧ NoDup (evs (add G a)) ∧
    (∀ t' i l1 l2, inG (add G a) (EEv t' i l1) → inG (add G a) (EEv t' i l2) → l1 = l2) ∧
    (∀ t' i l', inG (add G a) (EEv t' i l') → i < count (add G a) t') ∧
    (∀ t' j, j < count (add G a) t' → ∃ l', inG (add G a) (EEv t' j l')) ∧
    (∀ e1 e2, inG (add G a) e1 → inG (add G a) e2 → po e1 e2 → added_before (add G a) e1 e2).
  Proof.
    pose proof fresh as Hfresh.
    split; [|split; [|split; [|split; [|split]]]].
    - pose proof (wf_init _ Hwf) as Hh. unfold add; simpl. destruct (evs G); [by inversion Hh|]. done.
    - unfold add; simpl. apply NoDup_app. split; [by apply (wf_nodup _ Hwf)|]. split; [|apply NoDup_singleton].
      intros x Hx Hx'. apply list_elem_of_singleton in Hx'. subst. done.
    - intros t' i l1 l2 [H1|Ha]%inG_add [H2|Hb]%inG_add.
      + by eapply (wf_ids _ Hwf).
      + exfalso. unfold a in Hb; simplify_eq. apply (wf_idx _ Hwf) in H1. lia.
      + exfalso. unfold a in Ha; simplify_eq. apply (wf_idx _ Hwf) in H2. lia.
      + unfold a in *; by simplify_eq.
    - intros t' i l' [H1|Ha]%inG_add; rewrite count_add_a.
      + apply (wf_idx _ Hwf) in H1. destruct (decide (t' = t)) as [->|]; lia.
      + unfold a in Ha; simplify_eq. rewrite decide_True; lia.
    - intros t' j. rewrite count_add_a. destruct (decide (t' = t)) as [->|]; intros Hj.
      + destruct (decide (j = count G t)) as [->|].
        * exists l. apply inG_add. by right.
        * destruct ((wf_contig _ Hwf) t j) as [l' ?]; [lia|]. exists l'. apply inG_add. by left.
      + destruct ((wf_contig _ Hwf) t' j) as [l' ?]; [lia|]. exists l'. apply inG_add. by left.
    - intros e1 e2 [H1|Ha]%inG_add [H2|Hb]%inG_add Hpo.
      + by apply added_before_add, (wf_po_order _ Hwf).
      + subst e2. by apply added_before_add_last.
      + exfalso. subst e1. unfold a in Hpo. destruct e2 as [|t2 i2 l2]; simpl in Hpo; [done|].
        destruct Hpo as [-> ?]. apply (wf_idx _ Hwf) in H2. lia.
      + exfalso. subst. by apply po_irrefl in Hpo.
  Qed.

  Lemma wf_add_nonread : ¬ is_read a → graph_wf (add G a).
  Proof.
    intros Hnr. destruct wf_add_core as (?&?&?&?&?&?). constructor; try done.
    - intros r l' [Hr| ->]%inG_add Hl Hrd.
      + destruct ((wf_rf _ Hwf) r l' Hr Hl Hrd) as (w & ? & ? & ?). exists w. split; [done|]. split; [|done].
        apply inG_add; auto.
      + done.
    - intros e He. unfold add; simpl. by apply (wf_rf_none _ Hwf).
  Qed.

  Lemma wf_add_read w ll : is_read a → loc a = Some ll → inG G w → writes_loc w ll →
    graph_wf (setRF (add G a) a w).
  Proof.
    intros Hrd Hloc Hw Hwl. destruct wf_add_core as (?&?&?&?&?&?). constructor; try done.
    - intros r l' [Hr| ->]%inG_add Hl Hrd'; unfold setRF; simpl.
      + rewrite decide_False by (intros ->; by apply fresh).
        destruct ((wf_rf _ Hwf) r l' Hr Hl Hrd') as (w' & ? & ? & ?). exists w'. split; [done|]. split; [|done].
        apply inG_add; auto.
      + rewrite decide_True by done. exists w. split; [done|]. split; [apply inG_add; auto|].
        congruence.
    - intros e He. unfold setRF, add; simpl. rewrite decide_False; [by apply (wf_rf_none _ Hwf)|].
      intros ->. done.
  Qed.
End Add.

Lemma wf_setCO G wp a : graph_wf G → graph_wf (setCO G wp a).
Proof. intros Hwf. destruct Hwf as [? ? ? ? ? ? ? ?]. by constructor. Qed.

Lemma hist_setCO G wp a t i : hist (setCO G wp a) t i = hist G t i.
Proof. done. Qed.

Lemma hist_setRF_other G r w t i :
  (∀ k, k < i → ev_at G t k ≠ Some r) → hist (setRF G r w) t i = hist G t i.
Proof.
  intros Hr. apply hist_ext; [done|]. intros k e Hk He Hrd. unfold rval, setRF; simpl.
  rewrite decide_False; [done|]. intros ->. by eapply Hr.
Qed.
