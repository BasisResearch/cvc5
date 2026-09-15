; COMMAND-LINE: --incremental --user-pat=strict
; DISABLE-TESTER: unsat-core
; EXPECT: unknown
; EXPECT: (:speculation (:active true :rounds 4 :loop-threshold 5 :hypotheses ((instantiate :qid q :status rejected :reason "the instance simplifies to true" :quantifiers 1 :added 0 :rejected 1 :instances () :bodies ())) :loops ()))
; EXPECT: unsat
; EXPECT: (:speculation (:active true :rounds 1 :loop-threshold 5 :hypotheses ((instantiate :qid q2 :status applied :quantifiers 1 :added 1 :rejected 0 :instances ((a)) :bodies ((>= (f a) 1))) (instantiate :qid q2 :status rejected :reason "this instantiation of the formula was made already" :quantifiers 1 :added 0 :rejected 1 :instances () :bodies ())) :loops ()))
; A directed instance that simplifies to true adds nothing, and says so. One
; asked for twice is made once; the second request is rejected as made
; already.
(set-logic ALL)
(declare-fun f (Int) Int)
(declare-fun g (Int) Int)
(declare-const a Int)
(assert (forall ((x Int)) (! (or (> x 0) (> (f x) 0)) :pattern ((g x)) :qid q)))
(assert (forall ((x Int)) (! (> (f x) 0) :pattern ((g x)) :qid q2)))
(push)
(speculate :instantiate q ((x "5")))
(assert (< (f 5) 0))
(assert (< (f a) 0))
(check-sat)
(get-info :speculation)
(pop)
(push)
(speculate :instantiate q2 ((x "a")))
(speculate :instantiate q2 ((x "a")))
(assert (< (f a) 0))
(check-sat)
(get-info :speculation)
(pop)
