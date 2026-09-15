; COMMAND-LINE: --incremental --user-pat=strict
; DISABLE-TESTER: unsat-core
; DISABLE-TESTER: dump
; EXPECT: unknown
; EXPECT: unsat
; EXPECT: (:speculation (:active true :rounds 1 :loop-threshold 5 :hypotheses ((instantiate :qid ax_f :status applied :quantifiers 1 :added 1 :rejected 0 :instances ((a)) :bodies ((>= (f a) 1)))) :loops ()))
; EXPECT: unknown
; EXPECT: (:speculation (:active false :rounds 4 :loop-threshold 5 :hypotheses () :loops ()))
; EXPECT: unsat
; EXPECT: (:speculation (:active true :rounds 1 :loop-threshold 5 :hypotheses ((instantiate :qid ax_f :status applied :quantifiers 1 :added 1 :rejected 0 :instances (((+ 1 a))) :bodies ((>= (f (+ 1 a)) 1)))) :loops ()))
; EXPECT: unknown
; EXPECT: (:speculation (:active true :rounds 4 :loop-threshold 5 :hypotheses ((instantiate :qid ax_f :status mismatch :reason "no term for the variable i" :quantifiers 1 :added 0 :rejected 0 :instances () :bodies ())) :loops ()))
; EXPECT: unknown
; EXPECT: (:speculation (:active true :rounds 4 :loop-threshold 5 :hypotheses ((instantiate :qid ax_f :status mismatch :reason "the term for i has sort Bool, and i has sort Int" :quantifiers 1 :added 0 :rejected 0 :instances () :bodies ())) :loops ()))
; EXPECT: unknown
; EXPECT: (:speculation (:active true :rounds 4 :loop-threshold 5 :hypotheses ((instantiate :qid nope :status no-quantifier :quantifiers 0 :added 0 :rejected 0 :instances () :bodies ())) :loops ()))
; The only trigger of ax_f, (g i), has no instance, so e-matching cannot use
; ax_f. A directed instance at i := a closes the goal, in its own scope only:
; the next scope, without it, is unknown again, and no speculation is left
; active. The instance's term may be any term, here one not in the goal. A
; request that does not fit the formula's variables, or names no formula,
; changes nothing and says why.
(set-logic ALL)
(declare-fun f (Int) Int)
(declare-fun g (Int) Int)
(declare-const a Int)
(assert (forall ((i Int)) (! (> (f i) 0) :pattern ((g i)) :qid ax_f)))
(push)
(assert (< (f a) 0))
(check-sat)
(pop)
(push)
(speculate :instantiate ax_f ((i "a")))
(assert (< (f a) 0))
(check-sat)
(get-info :speculation)
(pop)
(push)
(assert (< (f a) 0))
(check-sat)
(get-info :speculation)
(pop)
(push)
(speculate :instantiate ax_f ((i "(+ a 1)")))
(assert (< (f (+ a 1)) 0))
(check-sat)
(get-info :speculation)
(pop)
(push)
(speculate :instantiate ax_f ((j "a")))
(assert (< (f a) 0))
(check-sat)
(get-info :speculation)
(pop)
(push)
(speculate :instantiate ax_f ((i "true")))
(assert (< (f a) 0))
(check-sat)
(get-info :speculation)
(pop)
(push)
(speculate :instantiate nope ((i "a")))
(assert (< (f a) 0))
(check-sat)
(get-info :speculation)
(pop)
