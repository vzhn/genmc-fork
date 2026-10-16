(** * Soundness (Section 4.3)

    Every graph the algorithm generates is a consistent full execution of
    the program. Consistency and fullness are part of [generated]; the
    content is that every visited graph is an execution of [P] (well
    formed, labels dictated by the program). The forward steps only append
    a fresh po-maximal event; the backward-revisit step restricts the
    graph, and the maximal-extension condition is what keeps the
    restriction an execution: no surviving read may read from a deleted
    write. *)
From Stdlib Require Import Classical.
From TruSt Require Export WF Algorithm.

Lemma head_filter_init (keep : Event → Prop) `{∀ e, Decision (keep e)} (l : list Event) :
  head l = Some EInit → keep EInit → head (filter keep l) = Some EInit.
Proof. destruct l as [|e es]; [done|]. simpl. intros [= ->] Hk. by rewrite filter_cons, decide_True. Qed.

Lemma tid_idx_some e : e ≠ EInit → ∃ t i, tid e = Some t ∧ idx e = Some i.
Proof. destruct e; [done|]. eauto. Qed.

Lemma po_of_tid_idx e1 e2 t i1 i2 :
  tid e1 = Some t → idx e1 = Some i1 → tid e2 = Some t → idx e2 = Some i2 → i1 < i2 → po e1 e2.
Proof. destruct e1, e2; simpl; intros; simplify_eq; auto. Qed.

Lemma wf_idx' G e t i : graph_wf G → inG G e → tid e = Some t → idx e = Some i → i < count G t.
Proof. intros Hwf He Ht Hi. destruct e; [done|]. simpl in *. simplify_eq. by eapply wf_idx. Qed.

Section Soundness.
  Context `{MemoryModel}.
  Variable P : Program.

  (** ** Forward steps *)

  Lemma exec_add_nonread G a :
    execution_of P G → next P G = Some a → ¬ is_read a → execution_of P (add G a).
  Proof.
    intros Hex Hn Hnr. destruct (next_shape _ _ _ Hn) as (t & lb & Ht & -> & Hs).
    pose proof (ex_wf _ _ Hex) as Hwf.
    constructor.
    - by apply wf_add_nonread.
    - intros t' i l' [Hin|Ha]%inG_add; [by eapply ex_tids|]. by simplify_eq.
    - intros t' i l' [Hin|Ha]%inG_add.
      + rewrite (hist_add_old G t lb); [by eapply ex_step|].
        destruct (decide (t' = t)) as [->|]; [right|by left].
        apply (wf_idx _ Hwf) in Hin. lia.
      + simplify_eq. rewrite (hist_add_old G t lb); [done|]. right. lia.
  Qed.

  Lemma exec_read G a l w :
    execution_of P G → next P G = Some a → is_read a → loc a = Some l →
    inG G w → writes_loc w l →
    execution_of P (setRF (add G a) a w).
  Proof.
    intros Hex Hn Hrd Hl Hw Hwl. destruct (next_shape _ _ _ Hn) as (t & lb & Ht & -> & Hs).
    pose proof (ex_wf _ _ Hex) as Hwf.
    constructor.
    - by eapply wf_add_read.
    - intros t' i l' [Hin|Ha]%inG_add; [by eapply ex_tids|]. by simplify_eq.
    - intros t' i l' [Hin|Ha]%inG_add.
      + rewrite hist_setRF_other.
        * rewrite (hist_add_old G t lb); [by eapply ex_step|].
          destruct (decide (t' = t)) as [->|]; [right|by left].
          apply (wf_idx _ Hwf) in Hin. lia.
        * intros k Hk He. apply ev_at_inG in He as (_ & Ht' & Hi). simpl in *. simplify_eq.
          apply (wf_idx _ Hwf) in Hin. lia.
      + simplify_eq. rewrite hist_setRF_other.
        * rewrite (hist_add_old G t lb); [done|]. right. lia.
        * intros k Hk He. apply ev_at_inG in He as (_ & Ht' & Hi). simpl in *. simplify_eq. lia.
  Qed.

  Lemma exec_setCO G wp a : execution_of P G → execution_of P (setCO G wp a).
  Proof.
    intros Hex. constructor.
    - apply wf_setCO, (ex_wf _ _ Hex).
    - intros t i l Hin. by eapply (ex_tids _ _ Hex).
    - intros t i l Hin. rewrite hist_setCO. by eapply (ex_step _ _ Hex).
  Qed.

  (** ** The backward revisit *)

  Section Revisit.
    Context (G : Graph) (t : Tid) (lb : Lab) (l : Loc) (r : Event).
    Let a := EEv t (count G t) lb.
    Context (Hex : execution_of P G).
    Context (Htid : t ∈ tids P).
    Context (Hs : step P t (hist G t (count G t)) = Some lb).
    Context (Hw : is_write a) (Hl : loc a = Some l).
    Context (Hr : inG G r) (Hrd : is_read r) (Hrl : loc r = Some l).
    Context (Hnp : ¬ porf (add G a) r a).
    Context (Hmax : ∀ e, deleted G r a e ∨ e = r → is_maximally_added (add G a) e a).

    Let keep := λ e, ¬ deleted G r a e.
    Let R := restrict G keep.

    Let Hwf : graph_wf G := ex_wf _ _ Hex.

    Lemma inG_addG e : inG G e → inG (add G a) e.
    Proof. intros. apply inG_add. by left. Qed.

    Lemma porf_po e1 e2 : inG G e1 → inG (add G a) e2 → po e1 e2 → porf (add G a) e1 e2.
    Proof. intros. apply tc_once. left. split; [by apply inG_addG|done]. Qed.

    Lemma keep_before_r e : inG G e → added_before G e r → keep e.
    Proof.
      intros He Hb (_ & [Hrb Hne] & _). apply Hne.
      eapply added_before_antisym; [apply (wf_nodup _ Hwf)|done|done].
    Qed.

    Lemma keep_porf e : porf (add G a) e a → keep e.
    Proof. intros Hp (_ & _ & Hn). done. Qed.

    Lemma keep_cases e : inG G e → keep e → added_before G e r ∨ porf (add G a) e a.
    Proof.
      intros He Hk. destruct (classic (porf (add G a) e a)) as [|Hnp']; [by right|].
      left. apply not_sbefore_before; [done|done|]. intros Hsb. apply Hk. by split.
    Qed.

    Lemma keep_init : keep EInit.
    Proof.
      intros (Hin & Hsb & _). exact (not_sbefore_init G r (wf_init _ Hwf) (wf_nodup _ Hwf) Hr Hsb).
    Qed.

    Lemma keep_po e1 e2 : inG G e1 → inG G e2 → po e1 e2 → keep e2 → keep e1.
    Proof.
      intros H1 H2 Hpo Hk. destruct (keep_cases e2 H2 Hk) as [Hb|Hp].
      - apply keep_before_r; [done|]. eapply added_before_trans; [apply (wf_nodup _ Hwf)| |done].
        by apply (wf_po_order _ Hwf).
      - apply keep_porf. eapply tc_l; [|done]. left. split; [by apply inG_addG|]. split; [by apply inG_addG|done].
    Qed.

    Lemma keep_thread_a e : inG G e → tid e = Some t → keep e.
    Proof.
      intros He Ht. apply keep_porf. apply porf_po; [done|apply inG_add; by right|].
      destruct e as [|t' i l']; [done|]. simpl in Ht. simplify_eq. simpl.
      split; [done|]. by apply (wf_idx _ Hwf) in He.
    Qed.

    Lemma deleted_after e : deleted G r a e → added_sbefore G r e.
    Proof. by intros (_ & ? & _). Qed.

    (** The heart of the matter: a kept read never reads from a deleted
        write, because such a write would not have been added maximally. *)
    Lemma keep_rf r' w : inG G r' → keep r' → rf G r' = Some w → inG G w → keep w.
    Proof.
      intros Hr' Hk Hrf Hw' Hdel.
      destruct (Hmax w (or_introl Hdel)) as [Hno _]. apply Hno. exists r'.
      split; [|done]. split; [by apply inG_addG|].
      destruct (keep_cases r' Hr' Hk) as [Hb|Hp]; [left|by right].
      apply added_before_add. eapply added_before_trans; [apply (wf_nodup _ Hwf)|done|].
      by apply deleted_after in Hdel as [].
    Qed.

    Lemma kept_thread_r_le e : inG G e → keep e → tid e = tid r → ∀ i, idx e = Some i → ∀ j, idx r = Some j → i ≤ j.
    Proof.
      intros He Hk Ht i Hi j Hj. destruct (decide (i ≤ j)) as [|Hgt]; [done|]. exfalso.
      destruct (tid e) as [te|] eqn:Hte; [|by destruct e].
      assert (Hpo : po r e) by (apply (po_of_tid_idx r e te j i); [by rewrite <-Ht|done|done|done|lia]).
      destruct (keep_cases _ He Hk) as [Hb|Hp].
      - pose proof ((wf_po_order _ Hwf) r e Hr He Hpo) as Hre.
        assert (e = r) as -> by (eapply added_before_antisym; [apply (wf_nodup _ Hwf)|done|done]).
        rewrite Hi in Hj. simplify_eq. lia.
      - apply Hnp. eapply tc_l; [|done]. left. split; [by apply inG_addG|]. split; [by apply inG_addG|done].
    Qed.

    Lemma inG_R e : inG R e ↔ inG G e ∧ keep e.
    Proof. apply inG_restrict. Qed.

    Lemma rf_R e : keep e → rf R e = rf G e.
    Proof. intros Hk. unfold R, restrict; simpl. by rewrite decide_True. Qed.

    Lemma count_R_spec t' i : (∃ l', inG R (EEv t' i l')) ↔ i < count R t'.
    Proof.
      (* kept serial numbers of a thread are downward closed, hence an initial segment *)
      pose proof (wf_nodup _ Hwf) as Hnd.
      assert (HndR : NoDup (evs R)) by (apply NoDup_filter, Hnd).
      assert (HidsR : ∀ i l1 l2, inG R (EEv t' i l1) → inG R (EEv t' i l2) → l1 = l2).
      { intros ? ? ? [? ?]%inG_R [? ?]%inG_R. by eapply (wf_ids _ Hwf). }
      assert (Hdown : ∀ i l', inG R (EEv t' i l') → ∀ j, j < i → ∃ l'', inG R (EEv t' j l'')).
      { intros i1 l' [Hin Hk]%inG_R j Hj.
        destruct ((wf_contig _ Hwf) t' j) as [l'' Hj']; [apply (wf_idx _ Hwf) in Hin; lia|].
        exists l''. apply inG_R. split; [done|]. eapply (keep_po _ (EEv t' i1 l')); [done|done| |done]. simpl. auto. }
      set (ids := (λ e, default 0 (idx e)) <$> thread_events R t').
      set (m := if decide (ids = []) then 0 else S (max_list ids)).
      assert (Hin_ids : ∀ n, (∃ l', inG R (EEv t' n l')) ↔ n ∈ ids).
      { intros n. unfold ids. rewrite list_elem_of_fmap. split.
        - intros [l' Hl']. exists (EEv t' n l'). split; [done|]. unfold thread_events. apply list_elem_of_filter. done.
        - intros (e & -> & He). unfold thread_events in He. apply list_elem_of_filter in He as [He ?].
          destruct e as [|te ie le]; [done|]. simpl in *. simplify_eq. eauto. }
      assert (Hspec : ∀ n, (∃ l', inG R (EEv t' n l')) ↔ n < m).
      { intros n. rewrite Hin_ids. unfold m. destruct (decide (ids = [])) as [Hnil|Hnn].
        - rewrite Hnil. split; [intros ?%elem_of_nil; done|lia].
        - split.
          + intros Hn. apply max_list_elem_of_le in Hn. lia.
          + intros Hn. pose proof (max_list_elem_of ids Hnn) as Hmx.
            apply Hin_ids in Hmx as [lm Hlm].
            destruct (decide (n = max_list ids)) as [->|]; [apply Hin_ids; eauto|].
            apply Hin_ids. eapply Hdown; [done|lia]. }
      rewrite Hspec. by rewrite (count_spec R t' m HndR HidsR Hspec).
    Qed.

    Lemma wf_R : graph_wf R.
    Proof.
      pose proof (wf_nodup _ Hwf) as Hnd.
      constructor.
      - apply head_filter_init; [apply (wf_init _ Hwf)|apply keep_init].
      - apply NoDup_filter, Hnd.
      - intros ? ? ? ? [? ?]%inG_R [? ?]%inG_R. by eapply (wf_ids _ Hwf).
      - intros t' i l' Hin. by apply count_R_spec; eauto.
      - intros t' j Hj. by apply count_R_spec.
      - intros e1 e2 [? ?]%inG_R [? ?]%inG_R Hpo. apply added_before_restrict; try done.
        by apply (wf_po_order _ Hwf).
      - intros r' l' [Hin Hk]%inG_R Hl' Hrd'.
        destruct ((wf_rf _ Hwf) r' l' Hin Hl' Hrd') as (w & Hrf & Hw' & Hwl).
        exists w. rewrite rf_R by done. split; [done|]. split; [|done].
        apply inG_R. split; [done|]. by eapply (keep_rf r' w).
      - intros e Hnr. unfold R, restrict; simpl. destruct (decide (keep e)); [|done].
        by apply (wf_rf_none _ Hwf).
    Qed.

    Lemma hist_R t' i :
      (∀ k, k < i → ∀ l', inG G (EEv t' k l') → keep (EEv t' k l')) →
      hist R t' i = hist G t' i.
    Proof.
      intros Hk. apply hist_ext.
      - intros k Hki. destruct (ev_at G t' k) as [e|] eqn:He.
        + apply ev_at_inG in He as (Hin & Ht' & Hi).
          destruct e as [|te ie le]; [done|]. simpl in *. simplify_eq.
          apply ev_at_Some; [apply (wf_nodup _ wf_R)|apply (wf_ids _ wf_R t')| |done|done].
          apply inG_R. split; [done|]. by apply Hk.
        + apply ev_at_None. intros l' Hl'%inG_R. destruct Hl' as [Hl' _].
          eapply ev_at_Some in Hl'; [|apply (wf_nodup _ Hwf)|apply (wf_ids _ Hwf t')|done|done]. congruence.
      - intros k e Hki He Hrd'. apply ev_at_inG in He as (Hin%inG_R & _ & _). destruct Hin as [_ Hk'].
        unfold rval. by rewrite rf_R.
    Qed.

    Lemma exec_R : execution_of P R.
    Proof.
      constructor.
      - apply wf_R.
      - intros ? ? ? [? ?]%inG_R. by eapply (ex_tids _ _ Hex).
      - intros t' i l' [Hin Hk]%inG_R. rewrite hist_R; [by eapply (ex_step _ _ Hex)|].
        intros k Hki l'' Hin'. eapply (keep_po _ (EEv t' i l')); [done|done| |done]. simpl. auto.
    Qed.

    Lemma count_R_t : count R t = count G t.
    Proof.
      apply (count_spec R t _ (wf_nodup _ wf_R) (wf_ids _ wf_R t)).
      intros i. split.
      - intros [l' [Hin _]%inG_R]. by apply (wf_idx _ Hwf) in Hin.
      - intros Hi. destruct ((wf_contig _ Hwf) t i Hi) as [l' Hl']. exists l'. apply inG_R.
        split; [done|]. by apply keep_thread_a.
    Qed.

    Lemma hist_R_t : hist R t (count G t) = hist G t (count G t).
    Proof. apply hist_R. intros. by apply keep_thread_a. Qed.

    Lemma tid_r_ne : tid r ≠ Some t.
    Proof.
      intros Ht. apply Hnp. apply porf_po; [done|apply inG_add; by right|].
      assert (Hne : r ≠ EInit) by (intros ->; done).
      destruct (tid_idx_some r Hne) as (tr & jr & Htr & Hjr).
      rewrite Htr in Ht. simplify_eq.
      apply (po_of_tid_idx r a t jr (count G t)); [done|done|done|done|].
      by eapply wf_idx'.
    Qed.

    Lemma writes_loc_a : writes_loc a l.
    Proof. unfold a in *. destruct lb; simpl in *; simplify_eq; done. Qed.

    (** The graph after the revisit (before the final [setCO]). *)
    Lemma exec_revisit : execution_of P (setRF (add R a) r a).
    Proof.
      pose proof wf_R as HwfR. pose proof exec_R as HexR.
      assert (Ha : a = EEv t (count R t) lb) by (unfold a; by rewrite count_R_t).
      assert (Hstep : step P t (hist R t (count R t)) = Some lb).
      { by rewrite count_R_t, hist_R_t. }
      constructor.
      - (* well-formedness: fresh event, then [r] reads from [a] *)
        destruct (wf_add_core R t lb HwfR) as (Hh&Hnd&Hids&Hidx&Hcg&Hpo).
        rewrite <-Ha in Hh, Hnd, Hids, Hidx, Hcg, Hpo.
        constructor; try done.
        + intros r' l' Hin' Hl' Hrd'.
          change (inG (add R a) r') in Hin'. apply inG_add in Hin' as [Hin|Hr'].
          * destruct (decide (r' = r)) as [->|Hne].
            { exists a. split; [unfold setRF; simpl; by rewrite decide_True|].
              split; [change (inG (add R a) a); apply inG_add; by right|].
              rewrite Hrl in Hl'. injection Hl' as <-. apply writes_loc_a. }
            destruct ((wf_rf _ HwfR) r' l' Hin Hl' Hrd') as (w & ? & ? & ?). exists w.
            split; [unfold setRF; simpl; by rewrite decide_False|].
            split; [change (inG (add R a) w); apply inG_add; by left|done].
          * subst r'. exfalso. assert (is_write a) as Hw' by exact Hw.
            unfold a in Hw', Hrd'. destruct lb; done.
        + intros e Hnr. unfold setRF, add; simpl. rewrite decide_False; [by apply (wf_rf_none _ HwfR)|].
          intros ->. done.
      - intros t' i l' Hin. change (inG (add R a) (EEv t' i l')) in Hin.
        apply inG_add in Hin as [Hin|Hin]; [by eapply (ex_tids _ _ HexR)|]. unfold a in Hin. by injection Hin as -> -> ->.
      - intros t' i l' Hin. change (inG (add R a) (EEv t' i l')) in Hin.
        apply inG_add in Hin as [Hin|Hin].
        + rewrite hist_setRF_other.
          * rewrite Ha, (hist_add_old R t lb); [by eapply (ex_step _ _ HexR)|].
            destruct (decide (t' = t)) as [->|]; [right|by left]. apply (wf_idx _ HwfR) in Hin. lia.
          * intros k Hk He. apply ev_at_inG in He as (He & Ht' & Hi).
            apply inG_add in He as [He%inG_R|He]; [|exfalso; apply tid_r_ne; by rewrite He].
            destruct He as [He Hk']. apply inG_R in Hin as [Hin Hki].
            pose proof (kept_thread_r_le (EEv t' i l') Hin Hki ltac:(simpl; by rewrite Ht') i eq_refl k Hi).
            lia.
        + unfold a in Hin. injection Hin as -> -> ->. rewrite hist_setRF_other.
          * rewrite Ha, (hist_add_old R t lb); [by rewrite hist_R_t|]. right. rewrite count_R_t. lia.
          * intros k Hk He. apply ev_at_inG in He as (_ & Ht' & _). by apply tid_r_ne.
    Qed.
  End Revisit.

  (** ** Every visited graph is an execution of [P] *)

  Theorem visit_execution G : visit P G → execution_of P G.
  Proof.
    induction 1 as [|G a l w Hv IH Hc Hn Hrd Hl Hw Hwl|G a l wp Hv IH Hc Hn Hw Hl Hwp Hwpl
                   |G a l r wp Hv IH Hc Hn Hw Hl Hr Hrd Hrl Hnp Hmax G' Hwp Hwpl|G a Hv IH Hc Hn Hnr Hnw Hne].
    - (* G0 *)
      constructor.
      + constructor; try done.
        * apply NoDup_singleton.
        * intros ? ? ? ? Hin. apply list_elem_of_singleton in Hin. done.
        * intros ? ? ? Hin. apply list_elem_of_singleton in Hin. done.
        * intros t j Hj. unfold count, thread_events in Hj. simpl in Hj. lia.
        * intros e1 e2 Hin1 Hin2 Hpo. apply list_elem_of_singleton in Hin1, Hin2. subst.
          by apply po_irrefl in Hpo.
        * intros ? ? Hin. apply list_elem_of_singleton in Hin. subst. done.
      + intros ? ? ? Hin. apply list_elem_of_singleton in Hin. done.
      + intros ? ? ? Hin. apply list_elem_of_singleton in Hin. done.
    - by eapply exec_read.
    - apply exec_setCO. apply exec_add_nonread; [done|done|]. intros Hrd. destruct a as [|? ? []]; done.
    - apply exec_setCO. destruct (next_shape _ _ _ Hn) as (t & lb & Ht & -> & Hs).
      eapply exec_revisit; eauto.
    - by apply exec_add_nonread.
  Qed.

  Theorem soundness G :
    generated P G → consistent G ∧ full P G ∧ execution_of P G.
  Proof.
    intros (Hv & Hc & Hf). split; [done|]. split; [done|]. by apply visit_execution.
  Qed.
End Soundness.
