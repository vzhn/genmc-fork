(** * The insertion order as a total order on a graph's events *)
From TruSt Require Export Lemmas.

(** Index characterization of [added_before]. *)
Lemma added_before_lookup G e1 e2 :
  added_before G e1 e2 ↔ ∃ i j, evs G !! i = Some e1 ∧ evs G !! j = Some e2 ∧ i ≤ j.
Proof.
  split.
  - intros [[-> He]|(l1&l2&l3&Heq)].
    + apply list_elem_of_lookup in He as [i Hi]. by exists i, i.
    + exists (length l1), (length l1 + S (length l2)). rewrite Heq.
      rewrite lookup_app_r by lia. rewrite Nat.sub_diag. simpl.
      rewrite lookup_app_r by lia.
      replace (length l1 + S (length l2) - length l1) with (S (length l2)) by lia. simpl.
      rewrite lookup_app_r by lia. rewrite Nat.sub_diag. simpl. auto with lia.
  - intros (i & j & Hi & Hj & Hij).
    destruct (decide (i = j)) as [->|Hne].
    + left. rewrite Hi in Hj. simplify_eq. split; [done|].
      unfold inG. by eapply list_elem_of_lookup_2.
    + right. assert (i < j) by lia.
      pose proof (take_drop_middle _ _ _ Hi) as Heq1.
      assert (drop (S i) (evs G) !! (j - S i) = Some e2) as Hj'.
      { rewrite lookup_drop. by replace (S i + (j - S i)) with j by lia. }
      pose proof (take_drop_middle _ _ _ Hj') as Heq2.
      exists (take i (evs G)), (take (j - S i) (drop (S i) (evs G))),
             (drop (S (j - S i)) (drop (S i) (evs G))).
      rewrite Heq2. by rewrite Heq1.
Qed.

Lemma added_before_refl G e : inG G e → added_before G e e.
Proof. intros. by left. Qed.

Lemma added_before_trans G e1 e2 e3 :
  NoDup (evs G) → added_before G e1 e2 → added_before G e2 e3 → added_before G e1 e3.
Proof.
  intros Hnd. rewrite !added_before_lookup.
  intros (i & j & Hi & Hj & ?) (j' & k & Hj' & Hk & ?).
  assert (j = j') as <- by (eapply NoDup_lookup; eauto).
  exists i, k. auto with lia.
Qed.

Lemma added_before_antisym G e1 e2 :
  NoDup (evs G) → added_before G e1 e2 → added_before G e2 e1 → e1 = e2.
Proof.
  intros Hnd. rewrite !added_before_lookup.
  intros (i & j & Hi & Hj & ?) (j' & i' & Hj' & Hi' & ?).
  assert (j = j') as <- by (eapply NoDup_lookup; eauto).
  assert (i = i') as <- by (eapply NoDup_lookup; eauto).
  assert (i = j) as -> by lia. congruence.
Qed.

Lemma added_before_total G e1 e2 :
  inG G e1 → inG G e2 → added_before G e1 e2 ∨ added_before G e2 e1.
Proof.
  intros [i Hi]%list_elem_of_lookup [j Hj]%list_elem_of_lookup.
  destruct (decide (i ≤ j)).
  - left. apply added_before_lookup. eauto.
  - right. apply added_before_lookup. exists j, i. auto with lia.
Qed.

Lemma not_sbefore_before G e1 e2 :
  inG G e1 → inG G e2 → ¬ added_sbefore G e1 e2 → added_before G e2 e1.
Proof.
  intros H1 H2 Hn. destruct (added_before_total G e1 e2) as [Hb|Hb]; try done.
  destruct (decide (e1 = e2)) as [->|Hne]; [by left|].
  exfalso. apply Hn. by split.
Qed.

Lemma added_before_init G e :
  head (evs G) = Some EInit → inG G e → added_before G EInit e.
Proof.
  intros Hh He. apply added_before_lookup.
  apply list_elem_of_lookup in He as [j Hj]. exists 0, j.
  split; [|auto with lia]. destruct (evs G); [done|]. simpl in *. congruence.
Qed.

Lemma not_sbefore_init G e :
  head (evs G) = Some EInit → NoDup (evs G) → inG G e → ¬ added_sbefore G e EInit.
Proof.
  intros Hh Hnd He [Hb Hne]. apply Hne. eapply added_before_antisym; eauto.
  by apply added_before_init.
Qed.
