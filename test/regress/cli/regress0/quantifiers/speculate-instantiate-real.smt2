; COMMAND-LINE: --incremental --user-pat=strict
; DISABLE-TESTER: unsat-core
; EXPECT: unsat
; EXPECT: (:speculation (:active true :rounds 1 :loop-threshold 5 :hypotheses ((instantiate :qid qr :status applied :quantifiers 1 :added 1 :rejected 0 :instances ((5.0)) :bodies ((not (>= (* (- 1) (r 5.0)) 0))))) :loops ()))
; EXPECT: unsat
; EXPECT: (:speculation (:active true :rounds 1 :loop-threshold 5 :hypotheses ((instantiate :qid qr :status applied :quantifiers 1 :added 1 :rejected 0 :instances (((to_real a))) :bodies ((not (>= (* (- 1) (r (to_real a))) 0))))) :loops ()))
; EXPECT: unknown
; EXPECT: (:speculation (:active true :rounds 4 :loop-threshold 5 :hypotheses ((instantiate :qid qi :status mismatch :reason "the term for i has sort Real, and i has sort Int" :quantifiers 1 :added 0 :rejected 0 :instances () :bodies ())) :loops ()))
; An integer term for a real variable means its real, as in an SMT-LIB term:
; "5" stands for 5.0, and an integer constant a for (to_real a). A real term
; for an integer variable is still a mismatch.
(set-logic ALL)
(declare-fun r (Real) Real)
(declare-fun g (Real) Real)
(declare-fun f (Int) Int)
(declare-fun h (Int) Int)
(declare-const a Int)
(assert (forall ((y Real)) (! (> (r y) 0.0) :pattern ((g y)) :qid qr)))
(assert (forall ((i Int)) (! (> (f i) 0) :pattern ((h i)) :qid qi)))
(push)
(speculate :instantiate qr ((y "5")))
(assert (< (r 5.0) 0.0))
(check-sat)
(get-info :speculation)
(pop)
(push)
(speculate :instantiate qr ((y "a")))
(assert (< (r (to_real a)) 0.0))
(check-sat)
(get-info :speculation)
(pop)
(push)
(speculate :instantiate qi ((i "0.5")))
(assert (< (f 0) 0))
(check-sat)
(get-info :speculation)
(pop)
