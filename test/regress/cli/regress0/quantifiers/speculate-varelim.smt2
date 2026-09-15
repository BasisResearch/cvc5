; COMMAND-LINE: --incremental --no-cbqi
; DISABLE-TESTER: unsat-core
; EXPECT: unsat
; EXPECT: (:speculation (:active true :rounds 1 :loop-threshold 5 :hypotheses ((instantiate :qid qp :status applied :quantifiers 1 :added 1 :rejected 0 :instances ((4)) :bodies ((not (>= (+ 4 (* (- 1) (f (+ 1 4)))) 1))))) :loops ()))
; EXPECT: unknown
; EXPECT: (:speculation (:active true :rounds 3 :loop-threshold 5 :hypotheses ((instantiate :qid q :status mismatch :reason "the formula binds no variable named x" :quantifiers 1 :added 0 :rejected 0 :instances () :bodies ())) :loops ()))
; The rewriter eliminates x, which is (+ y 1), from both formulas. The
; patterns mention x and go, but the :qid stays, so a hypothesis still finds
; each formula by name, over the variable that remains. The bodies differ:
; alpha-equivalent formulas are registered once, whatever their names.
(set-logic ALL)
(declare-fun f (Int) Int)
(declare-fun k (Int Int) Int)
(assert (forall ((x Int) (y Int)) (! (=> (= x (+ y 1)) (> (f x) y)) :qid q)))
(assert (forall ((x Int) (y Int)) (! (=> (= x (+ y 1)) (>= (f x) y)) :pattern ((k x y)) :qid qp)))
(push)
(speculate :instantiate qp ((y "4")))
(assert (< (f 5) 4))
(check-sat)
(get-info :speculation)
(pop)
(push)
(speculate :instantiate q ((x "5") (y "4")))
(assert (< (f 5) 4))
(check-sat)
(get-info :speculation)
(pop)
