; COMMAND-LINE: --incremental
; DISABLE-TESTER: unsat-core
; DISABLE-TESTER: model
; EXPECT: unsat
; EXPECT: (:speculation (:active true :rounds 1 :loop-threshold 5 :hypotheses ((trigger :qid loop :status pending :quantifiers 0 :pattern () :added 0 :instances () :materialized ())) :loops ()))
; Conflict-based instantiation closes the goal with pq before the round
; reaches e-matching, where hypotheses apply. The trigger was never tried,
; so it is pending, not no-quantifier: loop is asserted.
(set-logic ALL)
(declare-fun f (Int) Int)
(declare-fun g (Int) Int)
(declare-fun P (Int) Bool)
(declare-const a Int)
(assert (forall ((y Int)) (! (P y) :pattern ((P y)) :qid pq)))
(assert (not (P a)))
(assert (> (f a) 0))
(push)
(assert (forall ((x Int)) (! (> (f x) (f (g x))) :pattern ((g x)) :qid loop)))
(speculate :trigger loop ((x Int)) ("(f x)"))
(check-sat)
(get-info :speculation)
(pop)
