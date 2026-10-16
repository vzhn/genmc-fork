(** * List-level lemmas about graphs: insertion order, counts, [ev_at], [hist] *)
From TruSt Require Export Program.

(** ** Insertion order *)

Lemma added_before_inG_l G e1 e2 : added_before G e1 e2 → inG G e1.
Proof.
  intros [[-> ?]|(l1&l2&l3&Heq)]; [done|]. unfold inG. rewrite Heq.
  apply elem_of_app; right; left.
Qed.
Lemma added_before_inG_r G e1 e2 : added_before G e1 e2 → inG G e2.
Proof.
  intros [[-> ?]|(l1&l2&l3&Heq)]; [done|]. unfold inG. rewrite Heq.
  apply elem_of_app; right; right; apply elem_of_app; right; left.
Qed.

(** Appending keeps the order of old events and puts the new one last. *)
Lemma added_before_add G a e1 e2 :
  added_before G e1 e2 → added_before (add G a) e1 e2.
Proof.
  intros [[-> ?]|(l1&l2&l3&Heq)].
  - left. split; [done|]. apply inG_add; auto.
  - right. exists l1, l2, (l3 ++ [a]). unfold add; simpl. rewrite Heq.
    by rewrite <-!app_assoc, <-!app_comm_cons, <-app_assoc.
Qed.
Lemma added_before_add_last G a e :
  inG G e → added_before (add G a) e a.
Proof.
  intros He. apply list_elem_of_lookup in He as [i Hi].
  pose proof (take_drop_middle (evs G) i e Hi) as Heq.
  right. exists (take i (evs G)), (drop (S i) (evs G)), []. unfold add; simpl.
  rewrite <-Heq at 1. by rewrite <-app_assoc.
Qed.
Lemma app_singleton_inv {A} (l l' m : list A) (a x : A) :
  l ++ [a] = l' ++ x :: m →
  (m = [] ∧ x = a ∧ l = l') ∨ ∃ m', m = m' ++ [a] ∧ l = l' ++ x :: m'.
Proof.
  destruct m as [|y m _] using rev_ind; intros Heq.
  - left. apply app_inj_tail in Heq as [-> ->]. done.
  - right. exists m. rewrite app_comm_cons, app_assoc in Heq.
    apply app_inj_tail in Heq as [-> ->]. done.
Qed.

Lemma added_before_add_inv G a e1 e2 :
  ¬ inG G a → added_before (add G a) e1 e2 → e2 ≠ a → added_before G e1 e2.
Proof.
  intros Ha [[-> He]|(l1&l2&l3&Heq)] Hne.
  - left. split; [done|]. apply inG_add in He as [|]; [done|congruence].
  - right. unfold add in Heq; simpl in Heq.
    apply app_singleton_inv in Heq as [(Hnil & _ & _)|(m & Hm & ->)]; [by destruct l2|].
    symmetry in Hm.
    apply app_singleton_inv in Hm as [(-> & -> & ->)|(l3' & -> & ->)]; [done|].
    by exists l1, l2, l3'.
Qed.

(** Restriction keeps the relative order of kept events. *)
Lemma added_before_restrict G keep `{∀ e, Decision (keep e)} e1 e2 :
  added_before G e1 e2 → keep e1 → keep e2 → added_before (restrict G keep) e1 e2.
Proof.
  intros [[-> ?]|(l1&l2&l3&Heq)] H1 H2.
  - left. split; [done|]. by apply inG_restrict.
  - right. exists (filter keep l1), (filter keep l2), (filter keep l3).
    unfold restrict; simpl.
    rewrite Heq, filter_app, filter_cons, decide_True by done.
    by rewrite filter_app, filter_cons, decide_True by done.
Qed.
Lemma added_before_restrict_inv G keep `{∀ e, Decision (keep e)} e1 e2 :
  added_before (restrict G keep) e1 e2 → added_before G e1 e2.
Proof.
  intros [[-> He]|(l1&l2&l3&Heq)].
  - left. split; [done|]. by apply inG_restrict in He as [].
  - unfold restrict in Heq; simpl in Heq.
    (* [filter keep (evs G)] is a sublist of [evs G]; recover the decomposition *)
    assert (Hsub : (l1 ++ e1 :: l2 ++ e2 :: l3) `sublist_of` evs G).
    { rewrite <-Heq. apply sublist_filter. }
    clear Heq. right.
    apply sublist_app_l in Hsub as (k1 & rest & -> & _ & Hsub).
    apply sublist_cons_l in Hsub as (k2 & rest' & -> & Hsub).
    apply sublist_app_l in Hsub as (k3 & rest'' & -> & _ & Hsub).
    apply sublist_cons_l in Hsub as (k4 & rest''' & -> & _).
    exists (k1 ++ k2), (k3 ++ k4), rest'''. by rewrite <-!app_assoc.
Qed.

(** ** Counting thread events *)

Lemma thread_events_add_same G t a : tid a = Some t →
  thread_events (add G a) t = thread_events G t ++ [a].
Proof. intros. unfold thread_events, add; simpl. rewrite filter_app, filter_cons, decide_True; done. Qed.
Lemma thread_events_add_other G t a : tid a ≠ Some t →
  thread_events (add G a) t = thread_events G t.
Proof.
  intros. unfold thread_events, add; simpl. rewrite filter_app, filter_cons, decide_False, filter_nil; [|done].
  by rewrite app_nil_r.
Qed.
Lemma count_add_same G t a : tid a = Some t → count (add G a) t = S (count G t).
Proof. intros. unfold count. rewrite thread_events_add_same; [|done]. rewrite length_app; simpl; lia. Qed.
Lemma count_add_other G t a : tid a ≠ Some t → count (add G a) t = count G t.
Proof. intros. unfold count. by rewrite thread_events_add_other. Qed.
Lemma count_setRF G r w t : count (setRF G r w) t = count G t.
Proof. done. Qed.
Lemma count_setCO G wp a t : count (setCO G wp a) t = count G t.
Proof. done. Qed.

(** [count] is characterized by which serial numbers are present. *)
Lemma count_spec G t n :
  NoDup (evs G) →
  (∀ i l1 l2, inG G (EEv t i l1) → inG G (EEv t i l2) → l1 = l2) →
  (∀ i, (∃ l, inG G (EEv t i l)) ↔ i < n) →
  count G t = n.
Proof.
  intros Hnd Hids Hspec. unfold count.
  set (f := λ e : Event, default 0 (idx e)).
  assert (Hperm : f <$> thread_events G t ≡ₚ seq 0 n).
  { apply NoDup_Permutation.
    - apply NoDup_fmap_2_strong; [|by apply NoDup_filter].
      intros x y Hx Hy Hf. unfold thread_events in Hx, Hy.
      apply list_elem_of_filter in Hx as [Hx ?], Hy as [Hy ?].
      destruct x as [|tx ix lx]; [done|]. destruct y as [|ty iy ly]; [done|].
      simpl in *. assert (tx = t) by congruence. assert (ty = t) by congruence. subst.
      assert (ix = iy) by (unfold f in Hf; simpl in Hf; lia). subst.
      f_equal. eapply Hids; eauto.
    - apply NoDup_seq.
    - intros i. rewrite list_elem_of_fmap, elem_of_seq. split.
      + intros (e & -> & He). unfold thread_events in He.
        apply list_elem_of_filter in He as [He ?].
        destruct e as [|te ie le]; [done|]. simpl in *.
        assert (te = t) by congruence; subst.
        assert (ie < n) as ? by (apply Hspec; eauto). unfold f; simpl; lia.
      + intros Hi. assert (∃ l, inG G (EEv t i l)) as [l Hl] by (apply Hspec; lia).
        exists (EEv t i l). split; [done|]. unfold thread_events.
        apply list_elem_of_filter. done. }
  apply Permutation_length in Hperm. rewrite length_fmap, length_seq in Hperm. done.
Qed.

(** ** [ev_at] *)

Lemma ev_at_Some G t i e :
  NoDup (evs G) →
  (∀ i l1 l2, inG G (EEv t i l1) → inG G (EEv t i l2) → l1 = l2) →
  inG G e → tid e = Some t → idx e = Some i → ev_at G t i = Some e.
Proof.
  intros Hnd Hids He Ht Hi. unfold ev_at.
  destruct (list_find _ (evs G)) as [[k e']|] eqn:Hf; simpl.
  - apply list_find_Some in Hf as (Hk & [Ht' Hi'] & _).
    f_equal. destruct e as [|te ie le]; [done|]. destruct e' as [|te' ie' le']; [done|].
    simpl in *. simplify_eq. f_equal. symmetry. eapply Hids; [done|].
    unfold inG. by eapply list_elem_of_lookup_2.
  - exfalso. apply list_find_None in Hf.
    eapply Forall_forall in Hf; [|done]. simpl in Hf. tauto.
Qed.
Lemma ev_at_None G t i :
  (∀ l, ¬ inG G (EEv t i l)) → ev_at G t i = None.
Proof.
  intros Hno. unfold ev_at.
  destruct (list_find _ (evs G)) as [[k e']|] eqn:Hf; simpl; [|done].
  apply list_find_Some in Hf as (Hk & [Ht' Hi'] & _).
  destruct e' as [|te' ie' le']; [done|]. simpl in *. simplify_eq.
  exfalso. apply (Hno le'). unfold inG. by eapply list_elem_of_lookup_2.
Qed.
Lemma ev_at_inG G t i e : ev_at G t i = Some e → inG G e ∧ tid e = Some t ∧ idx e = Some i.
Proof.
  unfold ev_at. destruct (list_find _ (evs G)) as [[k e']|] eqn:Hf; simpl; [|done].
  intros [= <-]. apply list_find_Some in Hf as (Hk & [? ?] & _).
  split; [|done]. unfold inG. by eapply list_elem_of_lookup_2.
Qed.

(** ** [hist] only depends on [ev_at] and [rval] below the index *)

Lemma omap_ext_elem {A B} (f g : A → option B) (l : list A) :
  (∀ x, x ∈ l → f x = g x) → omap f l = omap g l.
Proof.
  induction l as [|x l IH]; intros Hfg; csimpl; [done|].
  rewrite Hfg by (apply elem_of_cons; auto).
  destruct (g x); [f_equal|]; apply IH; intros; apply Hfg; apply elem_of_cons; auto.
Qed.

Lemma hist_ext G G' t i :
  (∀ k, k < i → ev_at G t k = ev_at G' t k) →
  (∀ k e, k < i → ev_at G t k = Some e → is_read e → rval G e = rval G' e) →
  hist G t i = hist G' t i.
Proof.
  intros Hev Hrv. unfold hist. apply omap_ext_elem. intros k Hk%elem_of_seq.
  rewrite <-Hev by lia. destruct (ev_at G t k) as [e|] eqn:He; simpl; [|done].
  destruct (decide (is_read e)); [|done]. apply (Hrv k e); [lia|done|done].
Qed.
