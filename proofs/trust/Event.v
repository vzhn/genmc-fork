(** * Events (Definition 3.1 of the TruSt paper, POPL 2022)

    An event is either the initialization event or a thread event
    [EEv t i lab] with thread identifier [t], serial number [i] inside
    the thread, and label [lab]. The write kind of the paper is omitted:
    this development does not model RMW (exclusive) events yet. *)
From stdpp Require Export prelude list.

Definition Tid := nat.
Definition Idx := nat.
Definition Loc := nat.
Definition Val := nat.

Inductive Lab :=
| LW (l : Loc) (v : Val)   (* write of value [v] to [l] *)
| LR (l : Loc)             (* read from [l] *)
| LF                       (* fence *)
| LErr.                    (* error (assertion violation) *)

Inductive Event :=
| EInit
| EEv (t : Tid) (i : Idx) (lab : Lab).

Global Instance Lab_eq_dec : EqDecision Lab. Proof. solve_decision. Defined.
Global Instance Event_eq_dec : EqDecision Event. Proof. solve_decision. Defined.

Definition tid (e : Event) : option Tid :=
  match e with EInit => None | EEv t _ _ => Some t end.
Definition idx (e : Event) : option Idx :=
  match e with EInit => None | EEv _ i _ => Some i end.
Definition lab (e : Event) : option Lab :=
  match e with EInit => None | EEv _ _ l => Some l end.

Definition is_read (e : Event) : Prop :=
  match e with EEv _ _ (LR _) => True | _ => False end.
Definition is_write (e : Event) : Prop :=
  match e with EInit | EEv _ _ (LW _ _) => True | _ => False end.
Definition is_error (e : Event) : Prop :=
  match e with EEv _ _ LErr => True | _ => False end.

Global Instance is_read_dec e : Decision (is_read e).
Proof. destruct e as [|? ? []]; simpl; apply _. Defined.
Global Instance is_write_dec e : Decision (is_write e).
Proof. destruct e as [|? ? []]; simpl; apply _. Defined.
Global Instance is_error_dec e : Decision (is_error e).
Proof. destruct e as [|? ? []]; simpl; apply _. Defined.

(** The location accessed by a read or a thread write. [EInit] writes
    every location, so it has no single location; see [writes_loc]. *)
Definition loc (e : Event) : option Loc :=
  match e with
  | EEv _ _ (LW l _) => Some l
  | EEv _ _ (LR l) => Some l
  | _ => None
  end.

(** [writes_loc e l]: [e] is a write to location [l]. The initialization
    event writes [0] to every location. *)
Definition writes_loc (e : Event) (l : Loc) : Prop :=
  match e with
  | EInit => True
  | EEv _ _ (LW l' _) => l = l'
  | _ => False
  end.
Global Instance writes_loc_dec e l : Decision (writes_loc e l).
Proof. destruct e as [|? ? []]; simpl; apply _. Defined.

Definition wval (e : Event) : option Val :=
  match e with
  | EInit => Some 0
  | EEv _ _ (LW _ v) => Some v
  | _ => None
  end.

(** Program order: [EInit] precedes every thread event, and events of
    one thread are ordered by serial number. *)
Definition po (e1 e2 : Event) : Prop :=
  match e1, e2 with
  | EInit, EEv _ _ _ => True
  | EEv t1 i1 _, EEv t2 i2 _ => t1 = t2 ∧ i1 < i2
  | _, _ => False
  end.
Global Instance po_dec e1 e2 : Decision (po e1 e2).
Proof. destruct e1, e2; simpl; apply _. Defined.

Lemma po_irrefl e : ¬ po e e.
Proof. destruct e; simpl; [tauto|lia]. Qed.

Lemma po_trans e1 e2 e3 : po e1 e2 → po e2 e3 → po e1 e3.
Proof. destruct e1, e2, e3; simpl; intuition lia. Qed.
