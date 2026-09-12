; COMMAND-LINE: --incremental
; DISABLE-TESTER: unsat-core
; DISABLE-TESTER: dump
; EXPECT: sat
; An import whose skolem table is read before anything else has initialized
; the solver: rebuilding the skolem initializes it first.
(set-logic ALL)
(declare-fun f (Int) Int)
(import-instantiations k :skolems
  ("(w (forall ((j Int)) (> (f j) 0)) 0)"))
(check-sat)
