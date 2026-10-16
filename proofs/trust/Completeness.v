(** * Completeness and optimality (Theorems 4.1 and 4.2)

    Statements only, for now. Completeness: every consistent full
    execution of [P] is visited up to insertion order. Optimality: no
    graph is visited twice (up to insertion order). *)
From TruSt Require Export Algorithm.

Section Completeness.
  Context `{MemoryModel}.
  Variable P : Program.

  Theorem completeness G :
    execution_of P G → consistent G → full P G →
    ∃ G', gequiv G G' ∧ visit P G'.
  Proof.
  Admitted.

  (** Optimality, phrased on the exploration relation: a faithful statement
      needs the production sequence ("goes through at most one sequence of
      nested Visit calls"), which we will add once [visit] is refined into
      an explicit call tree. *)
End Completeness.
