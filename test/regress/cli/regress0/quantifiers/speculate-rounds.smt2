; COMMAND-LINE: --incremental --user-pat=strict --inst-max-rounds=3
; DISABLE-TESTER: unsat-core
; DISABLE-TESTER: model
; EXPECT: unknown
; EXPECT: (:speculation (:active true :rounds 3 :loop-threshold 5 :hypotheses ((trigger :qid loop :status applied :quantifiers 1 :pattern ((f x)) :added 3 :instances ((a) ((g a)) ((g (g a)))) :materialized ((forall ((x Int)) (! (>= (+ (f x) (* (- 1) (f (g x)))) 1) :pattern ((f x))))))) :loops ()))
; A speculative trigger (f x) on loop feeds itself: each instance brings a
; deeper (f (g x)) for the next round to match. Rounds whose only lemmas come
; from the trigger still count, so --inst-max-rounds stops it as it would a
; strategy's loop.
(set-logic ALL)
(declare-fun f (Int) Int)
(declare-fun g (Int) Int)
(declare-const a Int)
(assert (forall ((x Int)) (! (> (f x) (f (g x))) :pattern ((g x)) :qid loop)))
(push)
(speculate :trigger loop ((x Int)) ("(f x)"))
(assert (> (f a) 100))
(check-sat)
(get-info :speculation)
(pop)
