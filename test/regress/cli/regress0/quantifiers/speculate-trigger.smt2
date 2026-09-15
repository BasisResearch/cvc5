; COMMAND-LINE: --incremental --user-pat=strict
; DISABLE-TESTER: unsat-core
; DISABLE-TESTER: dump
; EXPECT: unsat
; EXPECT: (:speculation (:active true :rounds 1 :loop-threshold 5 :hypotheses ((trigger :qid ax_f :status applied :quantifiers 1 :pattern ((f i)) :added 1 :instances ((a)) :materialized ((forall ((i Int)) (! (>= (f i) 1) :pattern ((f i))))))) :loops ()))
; EXPECT: unknown
; EXPECT: (:speculation (:active false :rounds 4 :loop-threshold 5 :hypotheses () :loops ()))
; EXPECT: unknown
; EXPECT: (:speculation (:active true :rounds 4 :loop-threshold 5 :hypotheses ((trigger :qid ax_f :status unusable :reason "cannot match with (+ i 1): a trigger term applies an uninterpreted function to terms containing the formula's variables" :quantifiers 1 :pattern () :added 0 :instances () :materialized ())) :loops ()))
; EXPECT: unknown
; EXPECT: (:speculation (:active true :rounds 4 :loop-threshold 5 :hypotheses ((trigger :qid ax_f :status mismatch :reason "the formula binds no variable named j" :quantifiers 1 :pattern () :added 0 :instances () :materialized ())) :loops ()))
; EXPECT: unsat
; EXPECT: unknown
; EXPECT: (:speculation (:active true :rounds 4 :loop-threshold 5 :hypotheses ((trigger :qid ax_f :status applied :quantifiers 1 :pattern ((f i)) :added 0 :instances () :materialized ((forall ((i Int)) (! (>= (f i) 1) :pattern ((f i)))))) (block :qid ax_f :status applied :quantifiers 1 :fingerprint "(f _)" :blocked 4 :examples ((a)))) :loops ()))
; The only trigger of ax_f, (g i), has no instance. A speculative trigger
; (f i) matches (f a) and closes the goal in its scope; the next scope, without
; it, is unknown again. A pattern that cannot be a trigger, or one over a
; variable the formula does not bind, is not installed and says why. Two
; speculative triggers in one scope both apply. A block refuses a speculative
; trigger's matches like any trigger's: matched again each round, the same
; one is refused each round.
(set-logic ALL)
(declare-fun f (Int) Int)
(declare-fun g (Int) Int)
(declare-fun h (Int Int) Int)
(declare-const a Int)
(declare-const b Int)
(assert (forall ((i Int)) (! (> (f i) 0) :pattern ((g i)) :qid ax_f)))
(assert (forall ((i Int) (j Int)) (! (> (h i j) 0) :pattern ((g i) (g j)) :qid ax_h)))
(push)
(speculate :trigger ax_f ((i Int)) ("(f i)"))
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
(speculate :trigger ax_f ((i Int)) ("(+ i 1)"))
(assert (< (f a) 0))
(check-sat)
(get-info :speculation)
(pop)
(push)
(speculate :trigger ax_f ((j Int)) ("(f j)"))
(assert (< (f a) 0))
(check-sat)
(get-info :speculation)
(pop)
(push)
(speculate :trigger ax_f ((i Int)) ("(f i)"))
(speculate :trigger ax_h ((i Int) (j Int)) ("(h i j)"))
(assert (or (< (f a) 0) (< (h a b) 0)))
(check-sat)
(pop)
(push)
(speculate :trigger ax_f ((i Int)) ("(f i)"))
(speculate :block ax_f "(f _)")
(assert (< (f a) 0))
(check-sat)
(get-info :speculation)
(pop)
