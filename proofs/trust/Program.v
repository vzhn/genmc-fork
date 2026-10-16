(** * Programs and the [next] function (Section 4.1)

    We model a program as a collection of data-deterministic threads: the
    label of thread [t]'s [i]-th event is a function of the values that
    [t]'s earlier reads returned. This is the standard assumption behind
    stateless model checking, and it is what lets the algorithm restrict
    a graph to a porf-prefix and keep it an execution of the program.

    [next_P(G)] adds the [<next]-minimal available event. We fix [<next]
    to be thread order, which trivially includes po (events of one thread
    are added in serial-number order because [count] grows by one). *)
From TruSt Require Export Graph.

Record Program := mkP {
  tids : list Tid;                        (* thread identifiers, in [<next] order *)
  step : Tid → list Val → option Lab;     (* next label given the values read so far *)
}.

(** The event of thread [t] with serial number [i], if present. *)
Definition ev_at (G : Graph) (t : Tid) (i : Idx) : option Event :=
  list_find (λ e, tid e = Some t ∧ idx e = Some i) (evs G) ≫= (λ p, Some p.2).

(** Values returned by the reads of thread [t] with serial number below
    [i], in serial-number order. A read without an [rf] edge contributes
    nothing; well-formed graphs never have one. *)
Definition hist (G : Graph) (t : Tid) (i : Idx) : list Val :=
  omap (λ k, ev_at G t k ≫= (λ e, if decide (is_read e) then rval G e else None))
       (seq 0 i).

(** The next event of thread [t], if the thread has not finished. *)
Definition next_of (P : Program) (G : Graph) (t : Tid) : option Event :=
  step P t (hist G t (count G t)) ≫= (λ l, Some (EEv t (count G t) l)).

(** [next_P(G)]: the first thread (in [tids] order) with an available event. *)
Fixpoint next_from (P : Program) (G : Graph) (ts : list Tid) : option Event :=
  match ts with
  | [] => None
  | t :: ts' => match next_of P G t with
                | Some a => Some a
                | None => next_from P G ts'
                end
  end.
Definition next (P : Program) (G : Graph) : option Event := next_from P G (tids P).

(** [G] is full when no event is available. *)
Definition full (P : Program) (G : Graph) : Prop := next P G = None.

(** ** Executions of a program

    A graph is an execution of [P] when it is well formed and every thread
    event carries the label the program dictates given the values its
    thread read before it. This is the invariant soundness maintains. *)
Record execution_of (P : Program) (G : Graph) : Prop := {
  ex_wf : graph_wf G;
  ex_tids : ∀ t i l, inG G (EEv t i l) → t ∈ tids P;
  ex_step : ∀ t i l, inG G (EEv t i l) → step P t (hist G t i) = Some l;
}.

(** The initial graph. *)
Definition G0 : Graph := mkG [EInit] (λ _, None) (λ _ _, False).

Lemma next_from_in P G ts a : next_from P G ts = Some a → ∃ t, t ∈ ts ∧ next_of P G t = Some a.
Proof.
  induction ts as [|t ts IH]; simpl; [done|].
  destruct (next_of P G t) eqn:Ht; intros Hn.
  - inversion Hn; subst. exists t. split; [apply elem_of_cons; auto|done].
  - destruct (IH Hn) as (t' & ? & ?). exists t'. split; [apply elem_of_cons; auto|done].
Qed.

Lemma next_of_shape P G t a :
  next_of P G t = Some a → ∃ l, a = EEv t (count G t) l ∧ step P t (hist G t (count G t)) = Some l.
Proof.
  unfold next_of. destruct (step P t _) eqn:Hs; simpl; [|done].
  intros [= <-]. eauto.
Qed.

Lemma next_shape P G a :
  next P G = Some a → ∃ t l, t ∈ tids P ∧ a = EEv t (count G t) l ∧ step P t (hist G t (count G t)) = Some l.
Proof.
  intros Hn. destruct (next_from_in _ _ _ _ Hn) as (t & Ht & Hn').
  destruct (next_of_shape _ _ _ _ Hn') as (l & -> & ?). eauto.
Qed.
