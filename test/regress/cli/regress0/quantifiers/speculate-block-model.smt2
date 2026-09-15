; COMMAND-LINE: --incremental --finite-model-find
; DISABLE-TESTER: unsat-core
; DISABLE-TESTER: dump
; DISABLE-TESTER: model
; EXPECT: unknown
; EXPECT: (:reason-unknown incomplete)
; EXPECT: unsat
; Finite model finding answers sat once no instance is missing from its
; model. A block refuses instances the model needs, so with one the check
; is unknown, not sat; without it, the same query is unsat.
(set-logic UF)
(declare-sort U 0)
(declare-fun P (U) Bool)
(declare-const a U)
(push)
(assert (forall ((x U)) (! (P x) :qid q)))
(assert (not (P a)))
(speculate :block q "_")
(check-sat)
(get-info :reason-unknown)
(pop)
(push)
(assert (forall ((x U)) (! (P x) :qid q)))
(assert (not (P a)))
(check-sat)
(pop)
