; COMMAND-LINE: --incremental --user-pat=strict
; DISABLE-TESTER: unsat-core
; DISABLE-TESTER: dump
; EXPECT: unsat
; EXPECT: (:speculation (:active true :rounds 1 :loop-threshold 5 :hypotheses ((instantiate :qid ax_f :status applied :quantifiers 1 :added 1 :rejected 0 :instances ((b)) :bodies ((>= (f b) 1)))) :loops ()))
; EXPECT: unsat
; EXPECT: (:speculation (:active true :rounds 1 :loop-threshold 5 :hypotheses ((instantiate :qid ax_f :status applied :quantifiers 1 :added 1 :rejected 0 :instances (((+ 1 a))) :bodies ((>= (f (+ 1 a)) 1)))) :loops ()))
; A directed term goes through the same substitutions as the assertions, so
; a variable preprocessing eliminated, or a defined function, still means
; what it did in the input.
(set-logic ALL)
(declare-fun f (Int) Int)
(declare-fun g (Int) Int)
(declare-const a Int)
(declare-const b Int)
(define-fun h ((x Int)) Int (+ x 1))
(assert (forall ((i Int)) (! (> (f i) 0) :pattern ((g i)) :qid ax_f)))
(push)
(speculate :instantiate ax_f ((i "a")))
(assert (= a b))
(assert (< (f b) 0))
(check-sat)
(get-info :speculation)
(pop)
(push)
(speculate :instantiate ax_f ((i "(h a)")))
(assert (< (f (+ a 1)) 0))
(check-sat)
(get-info :speculation)
(pop)
