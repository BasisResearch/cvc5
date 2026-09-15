; COMMAND-LINE: --incremental
; DISABLE-TESTER: unsat-core
; DISABLE-TESTER: model
; EXPECT: unknown
; EXPECT: (:speculation (:active true :rounds 4 :loop-threshold 5 :hypotheses ((block :qid q :status applied :quantifiers 1 :fingerprint "(f (- 1))" :blocked 2 :examples (((- 1))))) :loops ()))
; EXPECT: unknown
; EXPECT: unsat
; A fingerprint names a constant as the printer spells it, (- 1) or
; (/ 1 2) included, and a block on it refuses the instance that closes
; each goal.
(set-logic ALL)
(declare-fun f (Int) Int)
(declare-fun r (Real) Real)
(assert (forall ((x Int)) (! (> (f x) 0) :pattern ((f x)) :qid q)))
(assert (forall ((y Real)) (! (> (r y) 0.0) :pattern ((r y)) :qid qr)))
(push)
(speculate :block q "(f (- 1))")
(assert (< (f (- 1)) 0))
(check-sat)
(get-info :speculation)
(pop)
(push)
(speculate :block qr "(r (/ 1 2))")
(assert (< (r 0.5) 0.0))
(check-sat)
(pop)
(push)
(speculate :block q "(f (- 1))")
(assert (< (f 1) 0))
(check-sat)
(pop)
