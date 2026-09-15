; COMMAND-LINE: --incremental --user-pat=strict
; DISABLE-TESTER: unsat-core
; EXPECT: (:speculation (:active false :rounds 0 :loop-threshold 5 :hypotheses () :loops ()))
; EXPECT: unsat
; EXPECT: unsat
; EXPECT: (:speculation (:active true :rounds 1 :loop-threshold 5 :hypotheses ((trigger :qid ax_f :status applied :quantifiers 1 :pattern ((f i)) :added 2 :instances ((a)) :materialized ((forall ((i Int)) (! (>= (f i) 1) :pattern ((f i)))))) (instantiate :qid ax_f :status applied :quantifiers 1 :added 2 :rejected 0 :instances ((b)) :bodies ((>= (f b) 1)))) :loops ()))
; Before any check the report shows the default loop threshold. A check in
; a scope nested inside a hypothesis's own applies it again after the inner
; pop; the report still names each formula, instance and materialized
; trigger once.
(set-logic ALL)
(get-info :speculation)
(declare-fun f (Int) Int)
(declare-fun g (Int) Int)
(declare-const a Int)
(declare-const b Int)
(assert (forall ((i Int)) (! (> (f i) 0) :pattern ((g i)) :qid ax_f)))
(push)
(speculate :trigger ax_f ((i Int)) ("(f i)"))
(speculate :instantiate ax_f ((i "b")))
(assert (< (f a) 0))
(push)
(check-sat)
(pop)
(check-sat)
(get-info :speculation)
(pop)
