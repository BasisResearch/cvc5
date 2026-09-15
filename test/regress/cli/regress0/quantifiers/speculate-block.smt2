; COMMAND-LINE: --incremental --inst-max-rounds=12
; DISABLE-TESTER: unsat-core
; DISABLE-TESTER: model
; EXPECT: unknown
; EXPECT: (:speculation (:active true :rounds 18 :loop-threshold 3 :hypotheses ((observe)) :loops ((loop :qid loop :instantiations 12 :directed 0 :rounds 12 :rises 11 :first-depth 0 :max-depth 11 :first-round 2 :last-round 18))))
; EXPECT: unknown
; EXPECT: (:speculation (:active true :rounds 6 :loop-threshold 3 :hypotheses ((block :qid loop :status applied :quantifiers 1 :fingerprint "(f (g (g _0)))" :blocked 2 :examples (((g (g a)))))) :loops ()))
; EXPECT: unsat
; EXPECT: unknown
; Each instance of loop introduces (f (g x)) in an atom, which the next round
; matches: a matching loop, which the round limit stops. Observed, the loop
; is reported; blocked from its second rung on, it is not, and fewer rounds
; run. Blocking only the rungs past the one a goal needs leaves the goal
; provable. A block lasts one scope.
(set-logic ALL)
(declare-fun f (Int) Int)
(declare-fun g (Int) Int)
(declare-const a Int)
(assert (forall ((x Int)) (! (> (f x) (f (g x))) :pattern ((f x)) :qid loop)))
(push)
(speculate :observe :loop-threshold 3)
(assert (> (f a) 100))
(check-sat)
(get-info :speculation)
(pop)
(push)
(speculate :block loop "(f (g (g _0)))" :loop-threshold 3)
(assert (> (f a) 100))
(check-sat)
(get-info :speculation)
(pop)
(push)
(speculate :block loop "(f (g (g _0)))")
(assert (> (f a) 100))
(assert (< (f (g a)) (f (g (g a)))))
(check-sat)
(pop)
(push)
(speculate :block loop "(f (g _))")
(assert (> (f a) 100))
(assert (< (f (g (g a))) (f (g (g (g a))))))
(check-sat)
(pop)
