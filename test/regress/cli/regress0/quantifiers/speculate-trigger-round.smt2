; COMMAND-LINE: --incremental --no-cbqi
; DISABLE-TESTER: unsat-core
; DISABLE-TESTER: dump
; DISABLE-TESTER: model
; EXPECT: unsat
; EXPECT: (:speculation (:active true :rounds 1 :loop-threshold 5 :hypotheses ((trigger :qid loop :status applied :quantifiers 1 :pattern ((f x)) :added 1 :instances ((a)) :materialized ((forall ((x Int)) (! (>= (+ (f x) (* (- 1) (f (g x)))) 1) :pattern ((f x))))))) :loops ()))
; EXPECT: unsat
; A speculative trigger (f x) on loop would feed itself without end. It is
; matched in the same round as the ordinary triggers, so pq's instance
; closes the goal in the first round, as it does when (f x) is loop's own
; pattern (the last check). --no-cbqi leaves pq to e-matching.
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
(push)
(assert (forall ((x Int)) (! (> (f x) (f (g x))) :pattern ((f x)) :qid loop)))
(check-sat)
(pop)
